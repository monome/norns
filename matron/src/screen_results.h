#pragma once

#include <stdint.h>

#include "screen.h"

typedef enum { 
    SCREEN_RESULTS_TEXT_EXTENTS,
    SCREEN_RESULTS_CURRENT_POINT,
    SCREEN_RESULTS_PEEK,
    SCREEN_RESULTS_CONTEXT_NEW,
    SCREEN_RESULTS_CONTEXT_GET_CURRENT,
    SCREEN_RESULTS_SURFACE_GET_EXTENTS,
} screen_results_t;

struct screen_results_common {
    uint32_t type;
};

struct screen_results_text_extents {
    struct screen_results_common common;
    // NB: cairo returns doubles,
    // and the actual lua call uses only w/h, casting to int
    float x_bearing;
    float y_bearing;
    float width;
    float height;
    float x_advance;
    float y_advance;
};

struct screen_results_current_point {
    struct event_common common;
    float x;
    float y;
};

struct screen_results_peek {
    struct event_common common;
    int w;
    int h;
    char *buf;
};

struct screen_results_context_new {
    struct event_common common;
    screen_context_t *context;
};

struct screen_results_context_get_current {
    struct event_common common;
    screen_context_t* context;
};

struct screen_results_surface_get_extents {
    struct event_common common;
    screen_surface_extents_t extents;
};

union screen_results_data { 
    uint32_t type;
    struct screen_results_text_extents text_extents;
    struct screen_results_current_point current_point;
    struct screen_results_peek peek;
    struct screen_results_context_new context_new;
    struct screen_results_context_get_current context_get_current;
    struct screen_results_surface_get_extents surface_get_extents;
};

extern void screen_results_init();
extern void screen_results_deinit();

extern union screen_results_data *screen_results_data_new(screen_results_t type);
extern void screen_results_free();

extern void screen_results_wait();
extern void screen_results_post(union screen_results_data *results);

extern union screen_results_data *screen_results_get();