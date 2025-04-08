#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <termios.h>
#include <lualib.h>
#include <sys/queue.h>

#include "device_common.h"
#include <libevdev/libevdev.h>

#define BUFFER_SIZE 256

struct dev_serial {
    struct dev_common base;
    int fd;
    struct termios oldtio, newtio;
    char line[BUFFER_SIZE];
    char* handler_id;
};

int dev_serial_init(void *self, lua_State *l);
void *dev_serial_start(void *self);
void dev_serial_deinit(void *self);

void dev_serial_send(struct dev_serial *d, const char *line, size_t len);

