#pragma once

#include <cairo.h>
#include <lualib.h>
#include <sys/queue.h>

typedef enum _matron_io_type {
  IO_SCREEN,
  IO_INPUT,
  IO_END,
} matron_io_type_t;

struct _matron_io;

typedef struct _io_ops {
  const char* name;
  matron_io_type_t type;
  size_t data_size;

  int (*config)(struct _matron_io* io, lua_State *l);
  int (*setup)(struct _matron_io* io);
  void (*destroy)(struct _matron_io* io);
} io_ops_t;

extern io_ops_t* io_types[];

int io_create(lua_State *l, io_ops_t *ops);
int io_setup_all(void);
void io_destroy_all(void);

typedef struct _matron_io {
  void *data;
  io_ops_t *ops;

  TAILQ_ENTRY(_matron_io) entries;
} matron_io_t;

extern TAILQ_HEAD(io_head, _matron_io) io_queue;

//// screen
struct _matron_fb;

typedef struct _screen_ops {
  io_ops_t io_ops;
  
  void (*paint)(struct _matron_fb *fb);
  void (*bind)(struct _matron_fb *fb, cairo_surface_t *surface);
} screen_ops_t;

typedef struct _matron_fb {
  matron_io_t io;

  cairo_t *cairo;
  cairo_surface_t *surface;
} matron_fb_t;

//// keys & encs
struct _matron_input;

typedef struct _input_ops {
  io_ops_t io_ops;

  void* (*poll)(void* input);
} input_ops_t;

typedef struct _matron_input {
  matron_io_t io;

  pthread_t poll_thread;
} matron_input_t;
