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

int dev_serial_init(void *self, lua_State *l) {
    struct dev_serial *d = (struct dev_serial *)self;
    struct dev_common *base = (struct dev_common *)self;

    d->fd = open(d->base.path, O_RDWR | O_NOCTTY | O_SYNC);
    if (d->fd < 0) {
        fprintf(stderr, "failed to open serial device: %s\n", d->base.path);
        return -1;
    }

    d->handler_id = strdup(lua_tostring(l, -2));
    fprintf(stderr, "dev_serial: TTY device %s at %s handled by %s\n", d->base.name, d->base.path, d->handler_id);

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
    lua_pop(l, 1);

    tcflush(d->fd, TCIFLUSH);
    if (tcsetattr(d->fd, TCSANOW, &d->newtio) < 0) {
        fprintf(stderr, "failed to initialize serial device: %s\n", d->base.path);
        close(d->fd);
        return -1;
    };

    base->start = &dev_serial_start;
    base->deinit = &dev_serial_deinit;
    return 0;
}

static void handle_event(void *dev, uint8_t id) {
    union event_data *ev = event_data_new(EVENT_SERIAL_EVENT);
    ev->serial_event.dev = dev;
    ev->serial_event.id = id;
    event_post(ev);
}

void *dev_serial_start(void *self) {
    struct dev_serial *di = (struct dev_serial *)self;
    struct dev_common *base = (struct dev_common *)self;

    ssize_t len;

    while (1) {
        len = read(di->fd, di->line, 255);
        if (len > 0) {
            di->line[len] = 0; // add null to end of string
            if (len > 1) {
                // fprintf(stderr,"serial> %s", di->line);
                handle_event(self, base->id);
            }
            len = 0;
        }
        usleep(1000); // 1ms
    }
    return NULL;
}

void dev_serial_deinit(void *self) {
    struct dev_serial *di = (struct dev_serial *)self;
    tcsetattr(di->fd, TCSANOW, &di->oldtio);
    close(di->fd);
    free(di->handler_id);
}

void dev_serial_send(struct dev_serial *d, const char *line) {
    // fprintf(stderr,"serial_send: %s",line);
    write(d->fd, line, strlen(line));
}
