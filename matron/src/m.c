#include <assert.h>
#include <pthread.h>
#include <search.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <monome.h>

#include "args.h"
#include "events.h"
#include "m.h"

#define M_DEV_PATH_SIZE 64

//--------------------
//--- static variables

struct m_dev;

struct m_dev_list {
  struct m_dev *head;
  struct m_dev *tail;
  int size;
} mdl
;
static int port = 8000;
static int id = 0;

//------------------------
//-- static functions
static int m_dev_list_add(struct m_dev *md);
static void m_dev_list_remove(struct m_dev *md);
static void m_handle_press(const monome_event_t *e, void *p);
static void m_handle_lift(const monome_event_t *e, void *p);
static void *m_dev_thread_start(void *md);

//-------------------------
//--- monome device class

// monome device data structure.
struct m_dev {
  struct m_dev *next;
  struct m_dev *prev;
  monome_t *m;
  int id;
  pthread_t tid;
  uint8_t data[4][64]; // led data by quad
  bool dirty[4];       // quad-dirty flags
};

// allocate and initialize a new device
static struct m_dev *m_dev_new(const char *node, int port) {
  struct m_dev *md;
  monome_t *m;

  printf("initializing monome device at %s:%d\n", node, port); fflush(stdout);
  m = monome_open(node, port);
  if(!m) {goto open_err;}
  md = (struct m_dev *) malloc( sizeof(struct m_dev) );

  md->m = m;
  md->id = id++;

  memset( md->data, 0, sizeof(md->data) );
  memset( md->dirty, 0, sizeof(md->dirty) );

  monome_register_handler(m, MONOME_BUTTON_DOWN, m_handle_press, md);
  monome_register_handler(m, MONOME_BUTTON_UP, m_handle_lift, md);

  m_dev_list_add(md);

  // FIXME: refactor
  pthread_attr_t attr;
  int s;
  s = pthread_attr_init(&attr);
  if(s) {
    goto attr_err;
  }

  s = pthread_create(&md->tid, &attr, &m_dev_thread_start, md);
  if(s) {
    goto thread_create_err;
  }
  pthread_attr_destroy(&attr);

  printf("successfully added monome device.\n"); fflush(stdout);
  return md;

attr_err:
  printf("m_init(): error on thread attributes \n"); fflush(stdout);
  return NULL;
thread_create_err:
  printf("m_init(): error creating thread\n"); fflush(stdout);
  pthread_attr_destroy(&attr);
  return NULL;
open_err:
  printf("error: couldn't open monome device at %s:%d\n", node, port);
  return NULL;
}

// free a device
static void m_dev_free(struct m_dev *md) {
  int ret = pthread_cancel(md->tid);
  if(ret) { printf("m_dev_free(): error in pthread_cancel()\n"); fflush(stdout); }
  ret = pthread_join(md->tid, NULL); // wait before free
  if(ret) { printf("m_dev_free(): error in pthread_cancel()\n"); fflush(stdout); }
  else { printf("m_dev_free(): thread finished\n"); fflush(stdout); }
  monome_close(md->m);               // libmonome frees the device pointer
  m_dev_list_remove(md);
  free(md);
}

// calculate quadrant number given x/y
static inline uint8_t m_dev_quad_idx(uint8_t x, uint8_t y) {
  return ( (y > 7) << 1 ) | (x > 7);
}
// calcalate offset into quad data given x/y
static inline uint8_t m_dev_quad_offset(uint8_t x, uint8_t y) {
  // quad data is row-first? i'm so confused
  return ( (y & 7) * 8 ) + (x & 7);
}

// set a given LED value
void m_dev_set_led(struct m_dev *md,
                   uint8_t x, uint8_t y, uint8_t val) {
  printf("m_dev_set_led: %d %s %d %d %d\n",
         md->id, m_dev_serial(md), x, y, val); fflush(stdout);
  uint8_t q = m_dev_quad_idx(x,y);
  md->data[q][m_dev_quad_offset(x,y)] = val;
  md->dirty[q] = true;
}

// transmit all dirty quads
void m_dev_refresh(struct m_dev *md) {
  static const int quad_xoff[4] = {0, 8, 0, 8};
  static const int quad_yoff[4] = {0, 0, 8, 8};
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

int m_dev_list_add(struct m_dev *md) {
  insque(md, mdl.tail);
  mdl.tail = md;
  if(mdl.size == 0) {
    mdl.head = md;
  }
  mdl.size++;
}

void m_dev_list_remove(struct m_dev *md) {
  if(mdl.tail == md) { mdl.tail = md->prev; }
  if(mdl.head == md) { mdl.head = NULL; }
  mdl.size--;
  remque(md);
}

static inline void
grid_key_event(const monome_event_t *e, void *p, int state) {
  struct m_dev *md = (struct m_dev *)p;
  union event_data *ev = event_data_new(EVENT_GRID_KEY);
  ev->grid_key.id = md->id;
  ev->grid_key.x = e->grid.x;
  ev->grid_key.y = e->grid.y;
  ev->grid_key.state = 1;
  event_post(ev);
}

void m_handle_press(const monome_event_t *e, void *p) {
  grid_key_event(e, p, 1);
}

void m_handle_lift(const monome_event_t *e, void *p) {
  grid_key_event(e, p, 0);
}

struct m_dev *m_dev_lookup_path(const char *path) {
  struct m_dev *md = mdl.head;
  const char *mpath;
  int i = 0;
  while(md != NULL) {
    mpath = monome_get_devpath(md->m);
    if(strcmp(path, mpath) == 0) {
      return md;
    } else {
    }
    md = md->next;
  }
  return NULL;
}

//------------------
//---- extern definitions

void m_init(void) {
  mdl.size = 0;
  mdl.head = NULL;
  mdl.tail = NULL;
}

void m_deinit(void) {
  struct m_dev *md = mdl.head;
  struct m_dev *md_next;
  for(int i = 0; i < mdl.size; i++) {
    md_next = md->next;
    m_dev_free(md);
    md = md_next;
  }
}

int m_dev_id(struct m_dev *md) {
  return md->id;
}

const char *m_dev_serial(struct m_dev *md) {
  return monome_get_serial(md->m);
}

const char *m_dev_name(struct m_dev *md) {
  return monome_get_friendly_name(md->m);
}

void m_add_device(const char *path) {
  struct m_dev *md = m_dev_new(path, port);
  if(md != NULL) {
    union event_data *ev = event_data_new(EVENT_MONOME_ADD);
    ev->monome_add.dev = md;
    event_post(ev);
  }
}

void m_remove_device(const char *path) {
  struct m_dev *md = m_dev_lookup_path(path);
  union { void *p; int i; } u;
  memset( &u, 0, sizeof(u) );
  if(md != NULL) {
    u.i = md->id;
    union event_data *ev = event_data_new(EVENT_MONOME_REMOVE);
    ev->monome_remove.id = md->id;
    event_post(ev);
    printf( "removing device at %08x , id %d, path %s\n",
            md, md->id, monome_get_devpath(md->m) ); fflush(stdout);
    m_dev_free(md);
  } else {
    printf("error: couldn't find monome device for path %s\n", path);
    fflush(stdout);
  }
}

void *m_dev_thread_start(void *md) {
  monome_event_loop( ( (struct m_dev *)md )->m );
}
