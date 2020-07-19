#include <assert.h>
#include <monome.h>
#include <pthread.h>
#include <search.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../args.h"
#include "../events.h"

#include "device.h"
#include "device_monome.h"

//------------------------
//-- static functions
static void dev_monome_handle_press(const monome_event_t *e, void *p);
static void dev_monome_handle_lift(const monome_event_t *e, void *p);
static void dev_monome_handle_encoder_delta(const monome_event_t *e, void *p);
static void dev_monome_handle_encoder_press(const monome_event_t *e, void *p);
static void dev_monome_handle_encoder_lift(const monome_event_t *e, void *p);

//-------------------------
//--- monome device class

// allocate and initialize a new device
int dev_monome_init(void *self) {
    struct dev_monome *md = (struct dev_monome *)self;
    struct dev_common *base = (struct dev_common *)self;
    const char *name;
    const char *serial;
    monome_t *m;

    m = monome_open(md->dev.path);

    if (!m) {
        fprintf(stderr, "error: couldn't open monome device at %s\n", md->dev.path);
        return -1;
    }

    md->m = m;

    memset(md->data, 0, sizeof(md->data));
    memset(md->dirty, 0, sizeof(md->dirty));

    if (monome_get_rows(md->m) == 0 && monome_get_cols(md->m) == 0) {
        md->type = DEVICE_MONOME_TYPE_ARC;
    } else {
        md->type = DEVICE_MONOME_TYPE_GRID;
    }

    monome_register_handler(m, MONOME_BUTTON_DOWN, dev_monome_handle_press, md);
    monome_register_handler(m, MONOME_BUTTON_UP, dev_monome_handle_lift, md);
    monome_register_handler(m, MONOME_ENCODER_DELTA, dev_monome_handle_encoder_delta, md);
    monome_register_handler(m, MONOME_ENCODER_KEY_DOWN, dev_monome_handle_encoder_press, md);
    monome_register_handler(m, MONOME_ENCODER_KEY_UP, dev_monome_handle_encoder_lift, md);

    // drop the name set by udev and use libmonome-provided name
    free(base->name);
    name = monome_get_friendly_name(m);
    base->name = strdup(name);

    serial = monome_get_serial(m);
    base->serial = strdup(serial);

    base->start = &dev_monome_start;
    base->deinit = &dev_monome_deinit;

    return 0;
}

// calculate quadrant number given x/y
static inline uint8_t dev_monome_quad_idx(uint8_t x, uint8_t y) {
    return ((y > 7) << 1) | (x > 7);
}
// calcalate offset into quad data given x/y
static inline uint8_t dev_monome_quad_offset(uint8_t x, uint8_t y) {
    return ((y & 7) * 8) + (x & 7);
}

// set grid rotation
void dev_monome_set_rotation(struct dev_monome *md, uint8_t rotation) {
    monome_set_rotation(md->m, rotation);
}

// set a given LED value
void dev_monome_grid_set_led(struct dev_monome *md, uint8_t x, uint8_t y, uint8_t val) {
    uint8_t q = dev_monome_quad_idx(x, y);
    md->data[q][dev_monome_quad_offset(x, y)] = val;
    md->dirty[q] = true;
}

// set a given LED value
void dev_monome_arc_set_led(struct dev_monome *md, uint8_t n, uint8_t x, uint8_t val) {
    md->data[n & 3][x & 63] = val;
    md->dirty[n & 3] = true;
}

// set all LEDs to value
void dev_monome_all_led(struct dev_monome *md, uint8_t val) {
    for (uint8_t q = 0; q < 4; q++) {
        for (uint8_t i = 0; i < 64; i++) {
            md->data[q][i] = val;
        }
        md->dirty[q] = true;
    }
}

// transmit all dirty quads
void dev_monome_refresh(struct dev_monome *md) {
    static const int quad_xoff[4] = {0, 8, 0, 8};
    static const int quad_yoff[4] = {0, 0, 8, 8};

    if (md->m == NULL) {
        return;
    }

    for (int quad = 0; quad < 4; quad++) {
        if (md->dirty[quad]) {
            if (md->type == DEVICE_MONOME_TYPE_ARC) {
                monome_led_ring_map(md->m, quad, md->data[quad]);
            } else {
                monome_led_level_map(md->m, quad_xoff[quad], quad_yoff[quad], md->data[quad]);
            }
            md->dirty[quad] = false;
        }
    }
}

// intensity
void dev_monome_intensity(struct dev_monome *md, uint8_t i) {
    if (i > 15)
        i = 15;
    monome_led_intensity(md->m, i);
}

//--------------------
//--- static functions

static inline void grid_key_event(const monome_event_t *e, void *p, int state) {
    struct dev_monome *md = (struct dev_monome *)p;
    union event_data *ev = event_data_new(EVENT_GRID_KEY);
    ev->grid_key.id = md->dev.id;
    ev->grid_key.x = e->grid.x;
    ev->grid_key.y = e->grid.y;
    ev->grid_key.state = state;
    event_post(ev);
}

void dev_monome_handle_press(const monome_event_t *e, void *p) {
    grid_key_event(e, p, 1);
}

void dev_monome_handle_lift(const monome_event_t *e, void *p) {
    grid_key_event(e, p, 0);
}

void dev_monome_handle_encoder_delta(const monome_event_t *e, void *p) {
    struct dev_monome *md = (struct dev_monome *)p;
    union event_data *ev = event_data_new(EVENT_ARC_ENCODER_DELTA);
    ev->arc_encoder_delta.id = md->dev.id;
    ev->arc_encoder_delta.number = e->encoder.number;
    ev->arc_encoder_delta.delta = e->encoder.delta;
    event_post(ev);
}

void dev_monome_handle_encoder_press(const monome_event_t *e, void *p) {
    struct dev_monome *md = (struct dev_monome *)p;
    union event_data *ev = event_data_new(EVENT_ARC_ENCODER_KEY);
    ev->arc_encoder_key.id = md->dev.id;
    ev->arc_encoder_key.number = e->encoder.number;
    ev->arc_encoder_key.state = 1;
    event_post(ev);
}

void dev_monome_handle_encoder_lift(const monome_event_t *e, void *p) {
    struct dev_monome *md = (struct dev_monome *)p;
    union event_data *ev = event_data_new(EVENT_ARC_ENCODER_KEY);
    ev->arc_encoder_key.id = md->dev.id;
    ev->arc_encoder_key.number = e->encoder.number;
    ev->arc_encoder_key.state = 0;
    event_post(ev);
}

int dev_monome_grid_rows(struct dev_monome *md) {
    return monome_get_rows(md->m);
}

int dev_monome_grid_cols(struct dev_monome *md) {
    return monome_get_cols(md->m);
}

void *dev_monome_start(void *md) {
    monome_event_loop(((struct dev_monome *)md)->m);
    return NULL;
}

void dev_monome_deinit(void *self) {
    struct dev_monome *md = (struct dev_monome *)self;
    monome_close(md->m); // libmonome frees the monome_t pointer
    md->m = NULL;
}
