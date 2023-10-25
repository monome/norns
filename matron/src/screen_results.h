#pragma once

#include <stdint.h>

typedef enum { 
    SCREEN_RESULTS_TEXT_EXTENTS,
    SCREEN_RESULTS_CURRENT_POINT,
    SCREEN_RESULTS_PEEK,
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
    char* buf;
};

union screen_results_data { 
    uint32_t type;
    struct screen_results_text_extents text_extents;
    struct screen_results_current_point current_point;
    struct screen_results_peek peek;
};

extern void screen_results_init();
extern void screen_results_deinit();

extern union screen_results_data* screen_results_data_new(screen_results_t type);
extern void screen_results_free();

extern void screen_results_wait();
extern void screen_results_post(union screen_results_data *results);

extern union screen_results_data* screen_results_get();