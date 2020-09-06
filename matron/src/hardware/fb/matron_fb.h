#pragma once

#include <cairo.h>

struct _matron_fb;

typedef struct _fb_ops {
    size_t data_size;
    const char* name;
    cairo_surface_t* (*init)(struct _matron_fb *fb);
    void (*destroy)(struct _matron_fb *fb);
    void (*paint)(struct _matron_fb *fb);
    void (*bind)(struct _matron_fb *fb, cairo_surface_t *surface);
} fb_ops_t;

typedef struct _matron_fb {
    cairo_t *cairo;
    cairo_surface_t *surface;
    void *data;
    fb_ops_t *ops;
} matron_fb_t;

extern fb_ops_t linux_fb_ops;
extern fb_ops_t json_fb_ops;
extern fb_ops_t sdl_fb_ops;
