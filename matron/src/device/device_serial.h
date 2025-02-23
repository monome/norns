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
    char line[255];
};

extern union dev* dev_serial_new(char *path, char *name, lua_State *l);
