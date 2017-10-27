#pragma once

typedef enum {
  // libmonome devices
  DEV_TYPE_MONOME = 0,
  // event devices (HID, &c)
  DEV_TYPE_INPUT = 1,
  // counter - unused, don't remove
  DEV_TYPE_COUNT,
  DEV_TYPE_INVALID
} device_t;

struct dev_common {
  // device type
  device_t type;
  // numerical id; unique over matron's lifetime
  uint32_t id;
  // thread id
  pthread_t tid;
  // path to device node in filesystem
  char *path;
  // serial string or similar
  char *serial;
  // human readable string
  char *name;
  // start function for pthread_create
  void * (*start)(void *self);
  // stop function
  void (*deinit)(void *self);
};
