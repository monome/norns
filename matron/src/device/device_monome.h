
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <monome.h>
#include "device_common.h"

// monome device data structure.
struct dev_monome {
  struct dev_common dev;
  monome_t *m;
  uint8_t data[4][64]; // led data by quad
  bool dirty[4];       // quad-dirty flags
};

// set a single led
extern void dev_monome_set_led(struct dev_monome *md,
                               uint8_t x, uint8_t y, uint8_t val);
// set all data for a quad
extern void dev_monome_set_quad(struct dev_monome *md,
                                uint8_t quad, uint8_t *data);
// transmit data for all dirty quads
extern void dev_monome_refresh(struct dev_monome *md);

// device management
extern int dev_monome_init(void *self);
extern void dev_monome_deinit(void *self);

extern void *dev_monome_start(void *self);
