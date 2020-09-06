#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <cairo.h>
#include <SDL2/SDL.h>

#include "hardware/fb/matron_fb.h"

typedef struct _cairo_sdl_fb_device {
    SDL_Window *window;
    SDL_Surface *window_surface;
    SDL_Surface *draw_surface;
} cairo_sdl_fb_device_t;

static cairo_surface_t* sdl_fb_init(matron_fb_t *fb);
static void sdl_fb_destroy(matron_fb_t *fb);
static void sdl_fb_paint(matron_fb_t *fb);
static void sdl_fb_bind(matron_fb_t *fb, cairo_surface_t *surface);

static void cairo_sdl_fb_surface_destroy(void *device);
static cairo_surface_t *cairo_sdl_fb_surface_create(cairo_sdl_fb_device_t *device);

fb_ops_t sdl_fb_ops = {
    .name = "sdl",
    .data_size = sizeof(cairo_sdl_fb_device_t),
    .init = sdl_fb_init,
    .destroy = sdl_fb_destroy,
    .paint = sdl_fb_paint,
    .bind = sdl_fb_bind,
};

cairo_surface_t* sdl_fb_init(matron_fb_t *fb) {
    return cairo_sdl_fb_surface_create((cairo_sdl_fb_device_t*)fb->data);
}

static void sdl_fb_destroy(matron_fb_t *fb) {
    cairo_destroy(fb->cairo);
    cairo_surface_destroy(fb->surface);
    free(fb->data);
}

static void sdl_fb_paint(matron_fb_t *fb) {
    cairo_sdl_fb_device_t *dev = fb->data;
    cairo_paint(fb->cairo);
    SDL_BlitSurface(dev->draw_surface, NULL, dev->window_surface, NULL);
    SDL_UpdateWindowSurface(dev->window);
}

static void sdl_fb_bind(matron_fb_t *fb, cairo_surface_t *surface) {
    cairo_set_operator(fb->cairo, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(fb->cairo, surface, 0, 0);
}

static void cairo_sdl_fb_surface_destroy(void *device) {
    cairo_sdl_fb_device_t *dev = (cairo_sdl_fb_device_t *)device;

    if (dev == NULL) {
        return;
    }
    SDL_FreeSurface(dev->draw_surface);
    SDL_DestroyWindow(dev->window);
    free(dev);
}

static cairo_surface_t *cairo_sdl_fb_surface_create(cairo_sdl_fb_device_t *device) {
    cairo_surface_t *surface;

    SDL_Init(SDL_INIT_VIDEO);
    device->window = SDL_CreateWindow("matron",
                                      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                      128, 64,
                                      SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    device->window_surface = SDL_GetWindowSurface(device->window);
    device->draw_surface = SDL_CreateRGBSurface(0,
                                               128, 64,
                                               16, 0xf800, 0x000007e0, 0x0000001f,
                                               0);
    surface = cairo_image_surface_create_for_data((unsigned char *)device->draw_surface->pixels,
                                                  CAIRO_FORMAT_RGB16_565, device->draw_surface->w, device->draw_surface->h,
                                                  cairo_format_stride_for_width(CAIRO_FORMAT_RGB16_565, device->draw_surface->w));
    cairo_surface_set_user_data(surface, NULL, device, &cairo_sdl_fb_surface_destroy);

    return surface;
}
