/*
 * screen.c
 *
 * oled
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h> 
#include <sys/ioctl.h>
#include <cairo.h>
#include <cairo-ft.h>

float c[16] = {0, 0.066666666666667, 0.13333333333333, 0.2, 0.26666666666667, 0.33333333333333, 0.4, 0.46666666666667, 0.53333333333333, 0.6, 0.66666666666667, 0.73333333333333, 0.8, 0.86666666666667, 0.93333333333333, 1};

cairo_surface_t *surface;
cairo_t *cr;
cairo_font_face_t * ct;
FT_Library value;
FT_Error status;
FT_Face face;

typedef struct _cairo_linuxfb_device {
    int fb_fd;
    unsigned char *fb_data;
    long fb_screensize;
    struct fb_var_screeninfo fb_vinfo;
    struct fb_fix_screeninfo fb_finfo;
} cairo_linuxfb_device_t;

/* Destroy a cairo surface */
void cairo_linuxfb_surface_destroy(void *device)
{
    cairo_linuxfb_device_t *dev = (cairo_linuxfb_device_t *)device;

    if (dev == NULL)
        return;

    munmap(dev->fb_data, dev->fb_screensize);
    close(dev->fb_fd);
    free(dev);
}

/* Create a cairo surface using the specified framebuffer */
cairo_surface_t *cairo_linuxfb_surface_create(const char *fb_name)
{
    cairo_linuxfb_device_t *device;
    cairo_surface_t *surface;

    /* Use fb0 if no fram buffer is specified */
    if (fb_name == NULL) {
        fb_name = "/dev/fb0";
    }

    device = malloc(sizeof(*device));
    if (!device) {
        perror("Error: cannot allocate memory\n");
        exit(1);
    }

    // Open the file for reading and writing
    device->fb_fd = open(fb_name, O_RDWR);
    if (device->fb_fd == -1) {
        perror("Error: cannot open framebuffer device");
        goto handle_allocate_error;
    }

    // Get variable screen information
    if (ioctl(device->fb_fd, FBIOGET_VSCREENINFO, &device->fb_vinfo) == -1) {
        perror("Error: reading variable information");
        goto handle_ioctl_error;
    }

    // Figure out the size of the screen in bytes
    device->fb_screensize = device->fb_vinfo.xres * device->fb_vinfo.yres
                            * device->fb_vinfo.bits_per_pixel / 8;

    // Map the device to memory
    device->fb_data = (unsigned char *)mmap(0, device->fb_screensize,
                                   PROT_READ | PROT_WRITE, MAP_SHARED,
                                   device->fb_fd, 0);
    if ((int)device->fb_data == -1) {
        perror("Error: failed to map framebuffer device to memory");
        goto handle_ioctl_error;
    }

    // Get fixed screen information
    if (ioctl(device->fb_fd, FBIOGET_FSCREENINFO, &device->fb_finfo) == -1) {
        perror("Error reading fixed information");
        goto handle_ioctl_error;
    }

    /* Create the cairo surface which will be used to draw to */
    surface = cairo_image_surface_create_for_data(device->fb_data,
                  CAIRO_FORMAT_RGB16_565,
                  device->fb_vinfo.xres,
                  device->fb_vinfo.yres,
                  cairo_format_stride_for_width(CAIRO_FORMAT_RGB16_565,
                                                device->fb_vinfo.xres));
    cairo_surface_set_user_data(surface, NULL, device,
                                &cairo_linuxfb_surface_destroy);

    return surface;

handle_ioctl_error:
    close(device->fb_fd);
handle_allocate_error:
    free(device);
    exit(1);
}

void screen_init(void) {
    surface = cairo_linuxfb_surface_create("/dev/fb1");
    cr = cairo_create(surface);

    char filename[256];
    // FIXME should be path relative to norns/
    snprintf(filename, 256, "%s/norns/assets/04B_03__.TTF", getenv("HOME"));
    //const char * filename = "/home/pi/slkscr.ttf";

    status = FT_Init_FreeType(&value);
    if(status != 0) {
    	fprintf(stderr, "Error %d opening library.\n", status);
	exit(EXIT_FAILURE);
    }
    status = FT_New_Face(value, filename, 0, &face);
    if(status != 0) {
    	fprintf(stderr, "Error %d opening %s.\n", status, filename);
	exit(EXIT_FAILURE);
    }
    ct = cairo_ft_font_face_create_for_ft_face(face, 0);

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_font_options_t *font_options;
    font_options = cairo_font_options_create();
    cairo_font_options_set_antialias(font_options,CAIRO_ANTIALIAS_SUBPIXEL);


    cairo_set_font_face (cr, ct);
    //cairo_select_font_face(cr, "cairo:sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_options(cr, font_options);
    cairo_set_font_size(cr, 8.0);
}


void screen_deinit(void) {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void screen_aa(int s) {
    if(s) { 
        cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
    } else {
        cairo_set_antialias(cr,CAIRO_ANTIALIAS_DEFAULT);
    }
}

void screen_level(int z) { 
    cairo_set_source_rgb(cr,c[z],c[z],c[z]);
}

void screen_line_width(long w) {
    cairo_set_line_width(cr,w);
}

void screen_move(int x, int y) {
    cairo_move_to(cr,x,y);
}

void screen_line(int x, int y) {
    cairo_line_to(cr,x,y);
}

void screen_stroke(void) {
    cairo_stroke(cr);
}

void screen_text(const char* s) {
    cairo_show_text(cr, s);
} 

void screen_clear(void) {
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}


