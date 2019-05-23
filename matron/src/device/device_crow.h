#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <termios.h>

#include <libevdev/libevdev.h>
#include "device_common.h"

//typedef uint8_t dev_vid_t;
//typedef uint8_t dev_pid_t;

#define CROW_BAUDRATE B115200

struct dev_crow {
    struct dev_common base;
		int fd;
    struct termios oldtio, newtio;
    //struct libevdev *dev;
    // identifiers
    //dev_vid_t vid;
    //dev_pid_t pid;
};

extern int dev_crow_init(void *self);
extern void *dev_crow_start(void *self);
extern void dev_crow_deinit(void *self);
