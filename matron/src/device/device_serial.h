#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <termios.h>
#include <lualib.h>
#include <sys/queue.h>

#include "device_common.h"
#include <libevdev/libevdev.h>

struct dev_serial {
    struct dev_common base;
    int fd;
    struct termios oldtio, newtio;
    char line[256];
    char* handler_id;
};

extern int dev_serial_init(void *self, lua_State *l);
extern void *dev_serial_start(void *self);
extern void dev_serial_deinit(void *self);

extern void dev_serial_send(struct dev_serial *d, const char *line);

