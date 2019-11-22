
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <monome.h>
#include "device_common.h"

typedef enum {
	DEVICE_MONOME_TYPE_GRID,
	DEVICE_MONOME_TYPE_ARC,
} device_monome_type_t;

// monome device data structure.
struct dev_monome {
    struct dev_common dev;
    device_monome_type_t type;
    monome_t *m;
    uint8_t data[4][64]; // led data by quad
    bool dirty[4];       // quad-dirty flags
};

// set a single grid led
extern void dev_monome_grid_set_led(struct dev_monome *md, uint8_t x, uint8_t y, uint8_t val);
// set a single arc led
extern void dev_monome_arc_set_led(struct dev_monome *md, uint8_t n, uint8_t x, uint8_t val);
// set all led
extern void dev_monome_all_led(struct dev_monome *md, uint8_t val);
// set all data for a quad
extern void dev_monome_set_quad(struct dev_monome *md,
                                uint8_t quad, uint8_t *data);
// transmit data for all dirty quads
extern void dev_monome_refresh(struct dev_monome *md);
extern int dev_monome_grid_rows(struct dev_monome *md);
extern int dev_monome_grid_cols(struct dev_monome *md);
// intensity
extern void dev_monome_intensity(struct dev_monome *md, uint8_t i);

// set grid rotation
extern void dev_monome_set_rotation(struct dev_monome *md, uint8_t val);

// device management
extern int dev_monome_init(void *self);
extern void dev_monome_deinit(void *self);

extern void *dev_monome_start(void *self);
