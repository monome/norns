#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <termios.h>

#include "device_common.h"
#include <libevdev/libevdev.h>

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
