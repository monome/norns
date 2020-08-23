#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "args.h"
#include "hardware/fb/matron_fb.h"

typedef struct _cairo_linuxfb_device {
    int fb_fd;
    unsigned char *fb_data;
    long fb_screensize;
    struct fb_var_screeninfo fb_vinfo;
    struct fb_fix_screeninfo fb_finfo;
} cairo_linuxfb_device_t;

static void linuxfb_destroy(matron_fb_t *f);
static void linuxfb_paint(matron_fb_t *f);
static void linuxfb_bind(matron_fb_t *f, cairo_surface_t *surface);

static void cairo_linuxfb_surface_destroy(void *device);
static cairo_surface_t *cairo_linuxfb_surface_create(cairo_linuxfb_device_t *device);

int linuxfb_init(matron_fb_t *fb) {
    fb->data = malloc(sizeof(cairo_linuxfb_device_t));
    if (!fb->data) {
        fprintf(stderr, "ERROR (screen - linuxfb) cannot allocate memory\n");
	return -1;
    }
    fb->surface = cairo_linuxfb_surface_create((cairo_linuxfb_device_t*)fb->data);
    if (!fb->surface) {
        free(fb->data);
        return -1;
    }
    fb->cairo = cairo_create(fb->surface);
    if (!fb->cairo) {
        cairo_surface_destroy(fb->surface);
	free(fb->data);
	return -1;
    }

    fb->destroy = &linuxfb_destroy;
    fb->paint = &linuxfb_paint;
    fb->bind = &linuxfb_bind;
    return 0;
}

static void linuxfb_destroy(matron_fb_t *fb) {
    cairo_destroy(fb->cairo);
    cairo_surface_destroy(fb->surface);
    free(fb->data);
}

static void linuxfb_paint(matron_fb_t *fb) {
    cairo_paint(fb->cairo);
}

static void linuxfb_bind(matron_fb_t *f, cairo_surface_t *surface) {
    cairo_set_operator(f->cairo, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(f->cairo, surface, 0, 0);
}

static void cairo_linuxfb_surface_destroy(void *device) {
    cairo_linuxfb_device_t *dev = (cairo_linuxfb_device_t *)device;

    if (dev == NULL) {
        return;
    }

    munmap(dev->fb_data, dev->fb_screensize);
    close(dev->fb_fd);
    free(dev);
}

static cairo_surface_t *cairo_linuxfb_surface_create(cairo_linuxfb_device_t *device) {
    cairo_surface_t *surface;

    const char *fb_name = args_framebuffer();

    // Open the file for reading and writing
    device->fb_fd = open(fb_name, O_RDWR);
    if (device->fb_fd == -1) {
        fprintf(stderr, "ERROR (screen) cannot open framebuffer device\n");
        goto handle_allocate_error;
    }

    // Get variable screen information
    if (ioctl(device->fb_fd, FBIOGET_VSCREENINFO, &device->fb_vinfo) == -1) {
        fprintf(stderr, "ERROR (screen) reading variable information\n");
        goto handle_ioctl_error;
    }

    // Figure out the size of the screen in bytes
    device->fb_screensize = device->fb_vinfo.xres * device->fb_vinfo.yres * device->fb_vinfo.bits_per_pixel / 8;

    // Map the device to memory
    device->fb_data =
        (unsigned char *)mmap(0, device->fb_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, device->fb_fd, 0);

    if (device->fb_data == (unsigned char *)-1) {
        fprintf(stderr, "ERROR (screen) failed to map framebuffer device to memory\n");
        goto handle_ioctl_error;
    }

    // Get fixed screen information
    if (ioctl(device->fb_fd, FBIOGET_FSCREENINFO, &device->fb_finfo) == -1) {
        fprintf(stderr, "ERROR (screen) reading fixed information\n");
        goto handle_ioctl_error;
    }

    /* Create the cairo surface which will be used to draw to */
    surface = cairo_image_surface_create_for_data(
        device->fb_data, CAIRO_FORMAT_RGB16_565, device->fb_vinfo.xres, device->fb_vinfo.yres,
        cairo_format_stride_for_width(CAIRO_FORMAT_RGB16_565, device->fb_vinfo.xres));
    cairo_surface_set_user_data(surface, NULL, device, &cairo_linuxfb_surface_destroy);

    fprintf(stderr, "screen: created linuxfb surface %s\n", fb_name);

    return surface;

handle_ioctl_error:
    close(device->fb_fd);
handle_allocate_error:
    free(device);
    return NULL;
}
