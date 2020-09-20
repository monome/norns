#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "args.h"
#include "hardware/fb/matron_fb.h"

typedef struct _cairo_linux_fb_device {
    int fb_fd;
    unsigned char *fb_data;
    long fb_screensize;
    struct fb_var_screeninfo fb_vinfo;
    struct fb_fix_screeninfo fb_finfo;
} cairo_linux_fb_device_t;

static cairo_surface_t* linux_fb_init(matron_fb_t *fb);
static void linux_fb_destroy(matron_fb_t *fb);
static void linux_fb_paint(matron_fb_t *fb);
static void linux_fb_bind(matron_fb_t *fb, cairo_surface_t *surface);

static void cairo_linux_fb_surface_destroy(void *device);
static cairo_surface_t *cairo_linux_fb_surface_create(cairo_linux_fb_device_t *device, const char *fb_name);

fb_ops_t linux_fb_ops = {
    .name = "fbdev",
    .data_size = sizeof(cairo_linux_fb_device_t),
    .init = linux_fb_init,
    .destroy = linux_fb_destroy,
    .paint = linux_fb_paint,
    .bind = linux_fb_bind,
};

cairo_surface_t* linux_fb_init(matron_fb_t *fb) {
    return cairo_linux_fb_surface_create((cairo_linux_fb_device_t*)fb->data,
                                         fb->config->dev);
}

static void linux_fb_destroy(matron_fb_t *fb) {
    cairo_destroy(fb->cairo);
    cairo_surface_destroy(fb->surface);
    free(fb->data);
}

static void linux_fb_paint(matron_fb_t *fb) {
    cairo_paint(fb->cairo);
}

static void linux_fb_bind(matron_fb_t *fb, cairo_surface_t *surface) {
    cairo_set_operator(fb->cairo, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(fb->cairo, surface, 0, 0);
}

static void cairo_linux_fb_surface_destroy(void *device) {
    cairo_linux_fb_device_t *dev = (cairo_linux_fb_device_t *)device;

    if (dev == NULL) {
        return;
    }

    munmap(dev->fb_data, dev->fb_screensize);
    close(dev->fb_fd);
    free(dev);
}

static cairo_surface_t *cairo_linux_fb_surface_create(cairo_linux_fb_device_t *device, const char *fb_name) {
    cairo_surface_t *surface;

    // Open the file for reading and writing
    device->fb_fd = open(fb_name, O_RDWR);
    if (device->fb_fd <= 0) {
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

    // Create the cairo surface which will be used to draw to
    surface = cairo_image_surface_create_for_data(
        device->fb_data, CAIRO_FORMAT_RGB16_565, device->fb_vinfo.xres, device->fb_vinfo.yres,
        cairo_format_stride_for_width(CAIRO_FORMAT_RGB16_565, device->fb_vinfo.xres));
    cairo_surface_set_user_data(surface, NULL, device, &cairo_linux_fb_surface_destroy);

    fprintf(stderr, "screen: created linux framebuffer surface %s\n", fb_name);

    return surface;

handle_ioctl_error:
    close(device->fb_fd);
handle_allocate_error:
    free(device);
    return NULL;
}

