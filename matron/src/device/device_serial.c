#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <termios.h>
#include <unistd.h>
#include <lualib.h>

#include "device.h"
#include "device_serial.h"
#include "events.h"
#include "lua_eval.h"

unsigned int get_sleep_us(speed_t ospeed, tcflag_t cflag);

int dev_serial_init(void *self, lua_State *l) {    
    struct dev_serial *d = (struct dev_serial *)self;
    struct dev_common *base = (struct dev_common *)self;

    d->fd = open(d->base.path, O_RDWR | O_NOCTTY | O_SYNC);
    if (d->fd < 0) {
        fprintf(stderr, "failed to open serial device: %s\n", d->base.path);
        return -1;
    }

    d->handler_id = strdup(lua_tostring(l, -1));
    fprintf(stderr, "dev_serial: TTY device %s at %s handled by %s\n", d->base.name, d->base.path, d->handler_id);

    // Initialize a new terminal config using the old config. The new config will
    // be passed to lua for overrides.
    tcgetattr(d->fd, &d->oldtio);
    d->newtio = d->oldtio;

    lua_getglobal(l, "_norns");
    lua_getfield(l, -1, "serial");
    lua_remove(l, -2);
    lua_getfield(l, -1, "configure");
    lua_remove(l, -2);

    lua_pushstring(l, d->handler_id);

    lua_createtable(l, 0, 8);
    lua_pushinteger(l, cfgetispeed(&d->oldtio));
    lua_setfield(l, -2, "ispeed");
    lua_pushinteger(l, cfgetospeed(&d->oldtio));
    lua_setfield(l, -2, "ospeed");
    lua_pushinteger(l, d->oldtio.c_iflag);
    lua_setfield(l, -2, "iflag");
    lua_pushinteger(l, d->oldtio.c_oflag);
    lua_setfield(l, -2, "oflag");
    lua_pushinteger(l, d->oldtio.c_cflag);
    lua_setfield(l, -2, "cflag");
    lua_pushinteger(l, d->oldtio.c_lflag);
    lua_setfield(l, -2, "lflag");
    lua_pushinteger(l, d->oldtio.c_line);
    lua_setfield(l, -2, "line");

    lua_createtable(l, NCCS, 0);
    for (int i = 0; i < NCCS; i++) {
        lua_pushinteger(l, d->oldtio.c_cc[i]);
        lua_rawseti(l, -2, i+1);
    }
    lua_setfield(l, -2, "cc");
    l_report(l, l_docall(l, 2, 1));

    if (!lua_istable(l, -1)) {
        fprintf(stderr, "serial handler config table expected, got %s\n", lua_typename(l, lua_type(l, -1)));
        close(d->fd);
        return -1;
    }

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

    d->read_timeout = get_sleep_us(d->newtio.c_ospeed, d->newtio.c_cflag);
    if (d->read_timeout == 0) {
        fprintf(stderr, "failed to determine serial read timeout from cflags\n");
        close(d->fd);
        return -1;
    }

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
        len = read(di->fd, di->line, max_read);
        if (len > 0) {
            di->line[len] = 0; // add null to end of string
            if (len > 1) {
                // fprintf(stderr,"serial> %s\n", di->line);
                handle_event(self, base->id);
            }
            len = 0;
        }
        usleep(di->read_timeout);
    }
    return NULL;
}

void dev_serial_deinit(void *self) {
    struct dev_serial *di = (struct dev_serial *)self;
    // Restore the tty device to the state it was in before connecting
    tcsetattr(di->fd, TCSANOW, &di->oldtio);
    close(di->fd);
    free(di->handler_id);
}

void dev_serial_send(struct dev_serial *d, const char *line) {
    // fprintf(stderr,"serial_send: %s",line);
    write(d->fd, line, strlen(line));
}

unsigned int get_baud_rate(speed_t ospeed) {
    switch (ospeed) {
        case B50:
            return 50;
        case B75:
            return 75;
        case B110:
            return 110;
        case B134:
            return 134;
        case B150:
            return 150;
        case B200:
            return 200;
        case B300:
            return 300;
        case B600:
            return 600;
        case B1200:
            return 1200;
        case B1800:
            return 1800;
        case B2400:
            return 2400;
        case B4800:
            return 4800;
        case B9600:
            return 9600;
        case B19200:
            return 19200;
        case B38400:
            return 38400;
        case B57600:
            return 57600;
        case B115200:
            return 115200;
        case B230400:
            return 230400;
        case B460800:
            return 460800;
        case B500000:
            return 500000;
        case B576000:
            return 576000;
        case B921600:
            return 921600;
        case B1000000:
            return 1000000;
        case B1152000:
            return 1152000;
        case B1500000:
            return 1500000;
        case B2000000:
            return 2000000;
        case B2500000:
            return 2500000;
        case B3000000:
            return 3000000;
        case B3500000:
            return 3500000;
        case B4000000:
            return 4000000;
        default:
            return 0;
    }
}

unsigned int get_character_size(tcflag_t cflag) {
    tcflag_t size = cflag & CSIZE;
    switch (size) {
        case CS8:
            return 8;
        case CS7:
            return 7;
        case CS6:
            return 6;
        default:
            return 5;
    }
}

unsigned int get_stop_bits(tcflag_t cflag) {
    if (cflag & CSTOPB) {
        return 2;
    }
    return 1;
}

unsigned int get_parity_bits(tcflag_t cflag) {
    if (cflag & PARENB) {
        return 1;
    }
    return 0;
}

unsigned int get_sleep_us(speed_t ospeed, tcflag_t cflag)
{
    unsigned int baud_rate = get_baud_rate(ospeed);
    unsigned int char_size = get_character_size(cflag);
    unsigned int stop_bits = get_stop_bits(cflag);
    unsigned int parity_bits =  get_parity_bits(cflag);

    unsigned int bits_per_char = char_size + stop_bits + parity_bits;
    double chars_per_second = baud_rate / bits_per_char;

    unsigned int us = floor(1000000 * max_read / chars_per_second);
    return us;
}
