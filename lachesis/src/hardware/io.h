#pragma once

#include <cairo.h>
#include <lualib.h>
#include <sys/queue.h>

typedef enum _lachesis_io_type {
  IO_SCREEN,
  IO_INPUT,
  IO_END,
} lachesis_io_type_t;

struct _lachesis_io;

typedef struct _io_ops {
  const char* name;
  lachesis_io_type_t type;
  size_t data_size;

  int (*config)(struct _lachesis_io* io, lua_State *l);
  int (*setup)(struct _lachesis_io* io);
  void (*destroy)(struct _lachesis_io* io);
} io_ops_t;

extern io_ops_t* io_types[];

int io_create(lua_State *l, io_ops_t *ops);
int io_setup_all(void);
void io_destroy_all(void);

typedef struct _lachesis_io {
  void *data;
  io_ops_t *ops;

  TAILQ_ENTRY(_lachesis_io) entries;
} lachesis_io_t;

extern TAILQ_HEAD(io_head, _lachesis_io) io_queue;

//// screen
struct _lachesis_fb;

typedef struct _screen_ops {
  io_ops_t io_ops;
  
  void (*paint)(struct _lachesis_fb *fb);
  void (*bind)(struct _lachesis_fb *fb, cairo_surface_t *surface);
} screen_ops_t;

typedef struct _lachesis_fb {
  lachesis_io_t io;

  cairo_t *cairo;
  cairo_surface_t *surface;
} lachesis_fb_t;

//// keys & encs
struct _lachesis_input;

typedef struct _input_ops {
  io_ops_t io_ops;

  void* (*poll)(void* input);
} input_ops_t;

typedef struct _lachesis_input {
  lachesis_io_t io;

  pthread_t poll_thread;
} lachesis_input_t;
