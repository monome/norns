#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "device_common.h"
#include <libevdev/libevdev.h>
#include <linux/input.h>

#define DEV_GUID_LEN 33

typedef uint8_t dev_vid_t;
typedef uint8_t dev_pid_t;
typedef uint16_t dev_code_t;

struct dev_hid {
    struct dev_common base;
    struct libevdev *dev;
    // identifiers
    dev_vid_t vid;
    dev_pid_t pid;
    char guid[DEV_GUID_LEN];
    // count of supported event types
    int num_types;
    // array of supported event types
    uint8_t *types;
    // count of supported event codes per event type
    int *num_codes;
    // arrays of supported event codes per event type
    dev_code_t **codes;
    // count of axis, i.e. analog inputs
    int num_abs;
    // arrays of axis propeties
    struct input_absinfo *absinfos;
    // list of axis codes
    dev_code_t *abs_codes;
};

extern int dev_hid_init(void *self);
extern void *dev_hid_start(void *self);
extern void dev_hid_deinit(void *self);
