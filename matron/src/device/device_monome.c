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

#define clamp_upper(value, max) ((value) < (max) ? (value) : (max))
#define clamp_lower(value, min) ((value) > (min) ? (value) : (min))
#define clamp_range(value, min, max) clamp_lower(min, clamp_upper(value, max))

// quad offset defaults
static const int quad_xoff[] = {0, 8, 0, 8};
static const int quad_yoff[] = {0, 0, 8, 8};

//------------------------
//-- static functions
static void dev_monome_handle_press(const monome_event_t *e, void *p);
static void dev_monome_handle_lift(const monome_event_t *e, void *p);
static void dev_monome_handle_encoder_delta(const monome_event_t *e, void *p);
static void dev_monome_handle_encoder_press(const monome_event_t *e, void *p);
static void dev_monome_handle_encoder_lift(const monome_event_t *e, void *p);
static void dev_monome_handle_tilt(const monome_event_t *e, void *p);

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

    md->rows = monome_get_rows(md->m);
    md->cols = monome_get_cols(md->m);

    if (md->rows == 0 && md->cols == 0) {
        fprintf(stderr, "monome device reporing zero rows/cols; assuming arc\n");
        md->type = DEVICE_MONOME_TYPE_ARC;
        md->quads = 4;

    } else {
        md->type = DEVICE_MONOME_TYPE_GRID;
        md->quads = (md->rows * md->cols) / 64;
        fprintf(stderr, "monome device appears to be a grid; rows=%d; cols=%d; quads=%d\n", md->rows, md->cols,
                md->quads);
    }

    memcpy(md->quad_xoff, quad_xoff, sizeof(quad_xoff));
    memcpy(md->quad_yoff, quad_yoff, sizeof(quad_yoff));

    monome_register_handler(m, MONOME_BUTTON_DOWN, dev_monome_handle_press, md);
    monome_register_handler(m, MONOME_BUTTON_UP, dev_monome_handle_lift, md);
    monome_register_handler(m, MONOME_ENCODER_DELTA, dev_monome_handle_encoder_delta, md);
    monome_register_handler(m, MONOME_ENCODER_KEY_DOWN, dev_monome_handle_encoder_press, md);
    monome_register_handler(m, MONOME_ENCODER_KEY_UP, dev_monome_handle_encoder_lift, md);
    monome_register_handler(m, MONOME_TILT, dev_monome_handle_tilt, md);

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
    // for 16x8 grid, only update relevant quads which must change with rotation
    if (md->quads == 2) {
        if (rotation == 0 || rotation == 2) {
            md->quad_xoff[1] = 8;
            md->quad_yoff[1] = 0;
        } else {
            md->quad_xoff[1] = 0;
            md->quad_yoff[1] = 8;
        }
    }
    monome_set_rotation(md->m, rotation);
}

// enable/disable grid tilt
void dev_monome_tilt_enable(struct dev_monome *md, uint8_t sensor) {
    monome_tilt_enable(md->m, sensor);
}
void dev_monome_tilt_disable(struct dev_monome *md, uint8_t sensor) {
    monome_tilt_disable(md->m, sensor);
}

// set a given LED value
void dev_monome_grid_set_led(struct dev_monome *md, uint8_t x, uint8_t y, int8_t val, bool rel) {
    uint8_t q = dev_monome_quad_idx(x, y);
    if (rel) {
        md->data[q][dev_monome_quad_offset(x, y)] =
            clamp_range(md->data[q][dev_monome_quad_offset(x, y)] + val, 0, 15);
    } else
        md->data[q][dev_monome_quad_offset(x, y)] = val;
    md->dirty[q] = true;
}

// set a given LED value
void dev_monome_arc_set_led(struct dev_monome *md, uint8_t n, uint8_t x, int8_t val, bool rel) {
		if (rel)
				md->data[n & 3][x & 63] = clamp_range(md->data[n & 3][x & 63] + val, 0, 15);
		else
				md->data[n & 3][x & 63] = val;
    md->dirty[n & 3] = true;
}

// set all LEDs to value
void dev_monome_all_led(struct dev_monome *md, int8_t val, bool rel) {
		if (rel) {
				for (uint8_t q = 0; q < md->quads; q++) {
						for (uint8_t i = 0; i < 64; i++) {
								md->data[q][i] = clamp_range(md->data[q][i] + val, 0, 15);
						}
						md->dirty[q] = true;
				}
		} else {
				for (uint8_t q = 0; q < md->quads; q++) {
						for (uint8_t i = 0; i < 64; i++) {
								md->data[q][i] = val;
						}
						md->dirty[q] = true;
				}
		}
}

// transmit all dirty quads
void dev_monome_refresh(struct dev_monome *md) {
    if (md->m == NULL) {
        return;
    }

    for (int quad = 0; quad < md->quads; quad++) {
        if (md->dirty[quad]) {
            if (md->type == DEVICE_MONOME_TYPE_ARC) {
                monome_led_ring_map(md->m, quad, md->data[quad]);
            } else {
                monome_led_level_map(md->m, md->quad_xoff[quad], md->quad_yoff[quad], md->data[quad]);
            }
            md->dirty[quad] = false;
        }
    }
}

// intensity
void dev_monome_intensity(struct dev_monome *md, uint8_t i) {
    if (i > 15)
        i = 15;
    if (md->type == DEVICE_MONOME_TYPE_ARC)
        monome_led_ring_intensity(md->m, i);
    else
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

void dev_monome_handle_tilt(const monome_event_t *e, void *p) {
    struct dev_monome *md = (struct dev_monome *)p;
    union event_data *ev = event_data_new(EVENT_GRID_TILT);
    ev->grid_tilt.id = md->dev.id;
    ev->grid_tilt.sensor = e->tilt.sensor;
    ev->grid_tilt.x = e->tilt.x;
    ev->grid_tilt.y = e->tilt.y;
    ev->grid_tilt.z = e->tilt.z;
    event_post(ev);
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
