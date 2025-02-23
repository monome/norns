#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <termios.h>
#include <unistd.h>
#include <lualib.h>

#include "device.h"
#include "device_serial.h"
#include "events.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

int dev_serial_init(void *self, lua_State *l);

union dev* dev_serial_new(char *path, char *name, lua_State *l) {
    union dev *d = calloc(1, sizeof(union dev));

    if (d == NULL) {
        return NULL;
    }

    // initialize the base class
    d->base.type = DEV_TYPE_SERIAL;
    d->base.path = path ? strdup(path) : NULL;
    d->base.name = name ? strdup(name) : NULL;

    if (dev_serial_init(d, l) < 0) {
        free(d);
        return NULL;
    }
    // dev_start
    return d;
}

int dev_serial_init(void *self, lua_State *l) {
    struct dev_serial *d = (struct dev_serial *)self;

    d->fd = open(d->base.path, O_RDWR | O_NOCTTY | O_SYNC);
    if (d->fd < 0) {
        fprintf(stderr, "failed to open serial device: %s\n", d->base.path);
        return -1;
    }

    tcgetattr(d->fd, &d->oldtio);
    d->newtio = d->oldtio;
    
    lua_getfield(l, -1, "ispeed");
    if (!lua_isnil(l, -1)) {
        speed_t ispeed = lua_tointeger(l, -1);
        cfsetispeed(&d->newtio, ispeed);
    }
    lua_pop(l, 1);

    lua_getfield(l, -1, "ospeed");
    if (!lua_isnil(l, -1)) {
        speed_t ospeed = lua_tointeger(l, -1);
        cfsetospeed(&d->newtio, ospeed);
    }
    lua_pop(l, 1);
    
    lua_getfield(l, -1, "iflag");
    if (!lua_isnil(l, -1)) {
        d->newtio.c_iflag = lua_tointeger(l, -1);
    }
    lua_pop(l, 1);
    
    lua_getfield(l, -1, "oflag");
    if (!lua_isnil(l, -1)) {
        d->newtio.c_oflag = lua_tointeger(l, -1);
    }
    lua_pop(l, 1);
    
    lua_getfield(l, -1, "cflag");
    if (!lua_isnil(l, -1)) {
        d->newtio.c_cflag = lua_tointeger(l, -1);
    }
    lua_pop(l, 1);
    
    lua_getfield(l, -1, "lflag");
    if (!lua_isnil(l, -1)) {
        d->newtio.c_lflag = lua_tointeger(l, -1);
    }
    lua_pop(l, 1);
    
    lua_getfield(l, -1, "line");
    if (!lua_isnil(l, -1)) {
        d->newtio.c_line = lua_tointeger(l, -1);
    }
    lua_pop(l, 1);
    
    lua_getfield(l, -1, "cc");
    for (int i = 0; i < NCCS; i++) {
        lua_pushinteger(l, i);
        lua_gettable(l, -2);
        if (!lua_isnil(l, -1)) {
            d->newtio.c_cc[i] = lua_tointeger(l, -1);
        }
        lua_pop(l, 1);
    }

    tcflush(d->fd, TCIFLUSH);
    tcsetattr(d->fd, TCSANOW, &d->newtio);
    return 0;
}

#pragma GCC diagnostic pop
