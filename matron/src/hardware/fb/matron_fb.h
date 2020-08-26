#pragma once

#include <cairo.h>

struct _matron_fb;

typedef struct _matron_fb {
    cairo_t *cairo;
    cairo_surface_t *surface;
    void *data;

    void (*destroy)(struct _matron_fb *fb);
    void (*paint)(struct _matron_fb *fb);
    void (*bind)(struct _matron_fb *fb, cairo_surface_t *surface);
} matron_fb_t;

extern int linux_fb_init(matron_fb_t *f);
extern int x11_fb_init(matron_fb_t *f);
extern int json_fb_init(matron_fb_t *f,
                        void (*write)(const char *data, size_t len), void (*flush)(void));
