#include <stdio.h>
#include <lualib.h>
#include "device_hid.h"
#include "device_crow.h"
#include "device_serial.h"

int dev_hid_init(void *self) {
    (void)self;
    fprintf(stderr, "HID devices not supported on Mac\n");
    return -1;
}

void *dev_hid_start(void *self) {
    (void)self;
    return NULL;
}

void dev_hid_deinit(void *self) {
    (void)self;
}

int dev_crow_init(void *self) {
    (void)self;
    fprintf(stderr, "Crow devices not supported on Mac\n");
    return -1;
}

void *dev_crow_start(void *self) {
    (void)self;
    return NULL;
}

void dev_crow_deinit(void *self) {
    (void)self;
}

void dev_crow_send(struct dev_crow *d, const char *line) {
    (void)d;
    (void)line;
}

int dev_serial_init(void *self, lua_State *l) {
    (void)self;
    (void)l;
    fprintf(stderr, "Serial devices not supported on Mac\n");
    return -1;
}

void *dev_serial_start(void *self) {
    (void)self;
    return NULL;
}

void dev_serial_deinit(void *self) {
    (void)self;
}

void dev_serial_send(struct dev_serial *d, const char *line, size_t len) {
    (void)d;
    (void)line;
    (void)len;
}
