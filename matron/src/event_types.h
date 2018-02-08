#pragma once

#include <stdint.h>
#include "oracle.h"

typedef enum {
    // unused (do not remove)
    EVENT_FIRST_EVENT = 0,
    // code to be executed by luavm
    EVENT_EXEC_CODE_LINE,
    // metro has fired
    EVENT_METRO,
    // gpio event
    EVENT_KEY,
    // gpio event
    EVENT_ENC,
    // battery level change
    EVENT_BATTERY,
    // power cable present
    EVENT_POWER,
    // libmonome device added
    EVENT_MONOME_ADD,
    // libmonome device removed
    EVENT_MONOME_REMOVE,
    // monome grid press/lift
    EVENT_GRID_KEY,
    // libevdev device added
    EVENT_HID_ADD,
    // libevdev device removed
    EVENT_HID_REMOVE,
    // hid gesture
    EVENT_HID_EVENT,
    // finished receiving audio engine list
    EVENT_ENGINE_REPORT,
    // finished receiving commands list
    EVENT_COMMAND_REPORT,
    // finished receiving polls list
    EVENT_POLL_REPORT,
    // polled value from crone
    EVENT_POLL_VALUE,
    // arbitrary polled data from crone
    EVENT_POLL_DATA,
    // polled waveform from crone
    EVENT_POLL_WAVE,
    // polled i/o VU levels from crone
    EVENT_POLL_IO_LEVELS,
    // quit the event loop
    EVENT_QUIT,
} event_t;

// a packed data structure for four volume levels
// each channel is represented by unsigned byte with audio taper:
// 255 == 0db
// each step represents 0.25db, down to -60db
// the
typedef union {
    uint8_t bytes[4];
    uint32_t uint;
} quad_levels_t;

struct event_common {
    uint32_t type;
    // could put timestamp here if we want
}; // +4

struct event_exec_code_line {
    struct event_common common;
    char *line;
}; // +4

struct event_monome_add {
    struct event_common common;
    void *dev;
}; // +4

struct event_monome_remove {
    struct event_common common;
    uint32_t id;
}; // +4

struct event_grid_key {
    struct event_common common;
    uint8_t id;
    uint8_t x;
    uint8_t y;
    uint8_t state;
}; // +4

struct event_hid_add {
    struct event_common common;
    void *dev;
}; // +4

struct event_hid_remove {
    struct event_common common;
    uint32_t id;
}; // +4

/// fixme: maybe break this up into hid_key, hid_abs, &c?
struct event_hid_event {
    struct event_common common;
    uint8_t id;
    uint8_t type;
    uint16_t code;
    int32_t value;
}; // +8

struct event_metro {
    struct event_common common;
    uint32_t id;
    uint32_t stage;
}; // +8

struct event_key {
    struct event_common common;
    uint8_t n;
    uint8_t val;
}; // +8

struct event_battery {
    struct event_common common;
    uint8_t percent;
}; // +8

struct event_power {
    struct event_common common;
    uint8_t present;
}; // +8

struct event_enc {
    struct event_common common;
    uint8_t n;
    int8_t delta;
}; // +8

struct event_poll_value {
    struct event_common common;
    uint32_t idx;
    float value;
}; // + 8

struct event_poll_io_levels {
    struct event_common common;
    uint32_t idx;
    quad_levels_t value;
}; // + 8

struct event_poll_data {
    struct event_common common;
    uint32_t idx;
    uint32_t size;
    uint8_t *data;
}; // + 12

// like DATA, but size is fixed
struct event_poll_wave {
    struct event_common common;
    uint32_t idx;
    uint8_t *data;
}; // + 8

union event_data {
    uint32_t type;
    struct event_exec_code_line exec_code_line;
    struct event_monome_add monome_add;
    struct event_monome_remove monome_remove;
    struct event_grid_key grid_key;
    struct event_hid_add hid_add;
    struct event_hid_remove hid_remove;
    struct event_hid_event hid_event;
    struct event_key key;
    struct event_enc enc;
    struct event_battery battery;
    struct event_power power;
    struct event_metro metro;
    struct event_poll_value poll_value;
    struct event_poll_data poll_data;
    struct event_poll_io_levels poll_io_levels;
    struct event_poll_wave poll_wave;
};
