#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <termios.h>

#include <libevdev/libevdev.h>
#include "device_common.h"

#define CROW_BAUDRATE B115200

struct dev_crow {
  struct dev_common base;
	int fd;
  struct termios oldtio, newtio;
	char line[255];
};

extern int dev_crow_init(void *self);
extern void *dev_crow_start(void *self);
extern void dev_crow_deinit(void *self);

extern void dev_crow_send(struct dev_crow *d, const char *line);
