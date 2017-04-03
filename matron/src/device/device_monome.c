#include <assert.h>
#include <pthread.h>
#include <search.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <monome.h>

#include "../args.h"
#include "../events.h"

#include "device.h"
#include "device_monome.h"

#define DEV_MONOME_PATH_SIZE 64

//--------------------
//--- static variables

static int port = 8000;

//------------------------
//-- static functions
static void dev_monome_handle_press(const monome_event_t *e, void *p);
static void dev_monome_handle_lift(const monome_event_t *e, void *p);

static void dev_monome_handle_press(const monome_event_t *e, void *p);
static void dev_monome_handle_lift(const monome_event_t *e, void *p);

//-------------------------
//--- monome device class

// allocate and initialize a new device
int dev_monome_init(void *self) {
  struct dev_monome *md = (struct dev_monome *)self;
  struct dev_common *base = (struct dev_common *)self;
  const char *name;
  const char *serial;
  monome_t *m;
  m = monome_open(md->dev.path, port);
  if(!m) {
    printf("error: couldn't open monome device at %s:%d\n", md->dev.path, port);
    return -1;
  }
  md->m = m;

  memset( md->data, 0, sizeof(md->data) );
  memset( md->dirty, 0, sizeof(md->dirty) );

  monome_register_handler(m, MONOME_BUTTON_DOWN, dev_monome_handle_press, md);
  monome_register_handler(m, MONOME_BUTTON_UP, dev_monome_handle_lift, md);

  name = monome_get_friendly_name(m);
  base->name = malloc(strlen(name) + 1);
  strcpy(base->name, name);

  serial = monome_get_serial(m);
  base->serial = malloc(strlen(serial) + 1);
  strcpy(base->serial, serial);

  base->start = &dev_monome_start;
  base->deinit = &dev_monome_deinit;
  return 0;
}

// calculate quadrant number given x/y
static inline uint8_t dev_monome_quad_idx(uint8_t x, uint8_t y) {
  return ( (y > 7) << 1 ) | (x > 7);
}
// calcalate offset into quad data given x/y
static inline uint8_t dev_monome_quad_offset(uint8_t x, uint8_t y) {
  return ( (y & 7) * 8 ) + (x & 7);
}

// set a given LED value
void dev_monome_set_led(struct dev_monome *md,
                        uint8_t x, uint8_t y, uint8_t val) {
  /* printf("dev_monome_set_led: %d %s %d %d %d\n", */
  /*             md->dev.id, md->dev.serial, x, y, val); fflush(stdout); */
  uint8_t q = dev_monome_quad_idx(x,y);
  md->data[q][dev_monome_quad_offset(x,y)] = val;
  md->dirty[q] = true;
}

// transmit all dirty quads
void dev_monome_refresh(struct dev_monome *md) {
  static const int quad_xoff[4] = {0, 8, 0, 8};
  static const int quad_yoff[4] = {0, 0, 8, 8};
  if (md->m == NULL) { return; }
  for(int quad = 0; quad < 4; quad++) {
    if(md->dirty[quad]) {
      monome_led_level_map(md->m,
                           quad_xoff[quad],
                           quad_yoff[quad],
                           md->data[quad]);
      md->dirty[quad] = false;
    }
  }
}

//--------------------
//--- static functions

static inline void
grid_key_event(const monome_event_t *e, void *p, int state) {
  struct dev_monome *md = (struct dev_monome *)p;
  union event_data *ev = event_data_new(EVENT_GRID_KEY);
  ev->grid_key.id = md->dev.id;
  ev->grid_key.x = e->grid.x;
  ev->grid_key.y = e->grid.y;
  ev->grid_key.state = state;
  event_post(ev);
}

void dev_monome_handle_press(const monome_event_t *e, void *p) {
  // printf("dev_monome_handle_press()\n"); fflush(stdout);
  grid_key_event(e, p, 1);
}

void dev_monome_handle_lift(const monome_event_t *e, void *p) {
  // printf("dev_monome_handle_lift()\n"); fflush(stdout);
  grid_key_event(e, p, 0);
}

int dev_monome_id(struct dev_monome *md) {
  return md->dev.id;
}

const char *dev_monome_serial(struct dev_monome *md) {
  return monome_get_serial(md->m);
}

const char *dev_monome_name(struct dev_monome *md) {
  return monome_get_friendly_name(md->m);
}

void *dev_monome_start(void *md) {
  monome_event_loop( ( (struct dev_monome *)md )->m );
  return NULL;
}

void dev_monome_deinit(void *self) {
  struct dev_monome *md = (struct dev_monome *)self;
  monome_close(md->m); // libmonome frees the monome_t pointer
  md->m = NULL;
}
