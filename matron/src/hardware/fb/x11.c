#include <stdio.h>
#include <stdlib.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include "hardware/fb/matron_fb.h"

typedef struct _cairo_x11_fb_device {
    Display *display;
    Drawable drawable;
    int screen;
    cairo_surface_t *src_surface;
} cairo_x11_fb_device_t;

static cairo_surface_t* x11_fb_init(matron_fb_t *fb);
static void x11_fb_destroy(matron_fb_t *fb);
static void x11_fb_paint(matron_fb_t *fb);
static void x11_fb_bind(matron_fb_t *fb, cairo_surface_t *surface);

static void cairo_x11_fb_surface_destroy(void *device);
static cairo_surface_t *cairo_x11_fb_surface_create(cairo_x11_fb_device_t *device);

fb_ops_t x11_fb_ops = {
    .name = "x11",
    .data_size = sizeof(cairo_x11_fb_device_t),
    .init = x11_fb_init,
    .destroy = x11_fb_destroy,
    .paint = x11_fb_paint,
    .bind = x11_fb_bind,
};

cairo_surface_t* x11_fb_init(matron_fb_t *fb) {
    return cairo_x11_fb_surface_create((cairo_x11_fb_device_t*)fb->data);
}

static void x11_fb_destroy(matron_fb_t *fb) {
    cairo_destroy(fb->cairo);
    cairo_surface_destroy(fb->surface);
    free(fb->data);
}

static void x11_fb_paint(matron_fb_t *fb) {
    cairo_paint(fb->cairo);
}

static void x11_fb_bind(matron_fb_t *fb, cairo_surface_t *surface) {
    cairo_x11_fb_device_t *device = fb->data;
    device->src_surface = surface;
    cairo_set_operator(fb->cairo, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(fb->cairo, surface, 0, 0);
}

static void cairo_x11_fb_surface_destroy(void *device) {
    cairo_x11_fb_device_t *dev = (cairo_x11_fb_device_t *)device;

    if (dev == NULL) {
        return;
    }

    XCloseDisplay(dev->display);
    free(dev);
}

static cairo_surface_t *cairo_x11_fb_surface_create(cairo_x11_fb_device_t *device) {
    cairo_surface_t *surface;
    
    device->display = XOpenDisplay(NULL);
    if (!device->display) {
        fprintf(stderr, "ERROR: (screen - x11) could not open display\n");
        return NULL;
    }
    device->screen = DefaultScreen(device->display);
    int width = 128;
    int height = 64;
    device->drawable = XCreateSimpleWindow(device->display, DefaultRootWindow(device->display),
                                           0, 0, width, height, 0, 0, 0);
    XSelectInput(device->display, device->drawable, ButtonPressMask | KeyPressMask);
    XMapWindow(device->display, device->drawable);
    surface = cairo_xlib_surface_create(device->display, device->drawable,
                                        DefaultVisual(device->display, device->screen),
                                        width, height);
    cairo_xlib_surface_set_size(surface, width, height);
    cairo_surface_set_user_data(surface, NULL, device, &cairo_x11_fb_surface_destroy);
    fprintf(stderr, "screen: created x11 surface\n");
    return surface;
}
