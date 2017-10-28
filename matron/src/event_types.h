#pragma once

#include <stdint.h>
#include "oracle.h"

typedef enum {
    // unused (do not remove)
    EVENT_FIRST_EVENT = 0,
    // code to be executed by luavm
    EVENT_EXEC_CODE_LINE,
    // timer has fired
    EVENT_TIMER,
    // libmonome device added
    EVENT_MONOME_ADD,
    // libmonome device removed
    EVENT_MONOME_REMOVE,
    // monome grid press/lift
    EVENT_GRID_KEY,
    // libevdev device added
    EVENT_INPUT_ADD,
    // libevdev device removed
    EVENT_INPUT_REMOVE,
    // input gesture
    EVENT_INPUT_EVENT,
    // finished receiving audio engine list
    EVENT_ENGINE_REPORT,
    // finished receiving commands list
    EVENT_COMMAND_REPORT,
    // finished receiving polls list
    EVENT_POLL_REPORT,
    // polled value from crone
    EVENT_POLL_VALUE,
    // polled data from crone
    EVENT_POLL_DATA,
    // quit the event loop
    EVENT_QUIT,
} event_t;

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

struct event_input_add {
    struct event_common common;
    void *dev;
}; // +4

struct event_input_remove {
    struct event_common common;
    uint32_t id;
}; // +4

/// fixme: maybe break this up into input_key, input_abs, &c?
struct event_input_event {
    struct event_common common;
    uint8_t id;
    uint8_t type;
    uint16_t code;
    int32_t value;
}; // +8

struct event_timer {
    struct event_common common;
    uint32_t id;
    uint32_t stage;
}; // +8

struct event_poll_value {
    struct event_common common;
    uint32_t idx;
    float value;
}; // + 8

struct event_poll_data {
    struct event_common common;
    uint32_t idx;
    uint32_t size;
    uint8_t *data;
}; // + 12

union event_data {
    uint32_t type;
    struct event_exec_code_line exec_code_line;
    struct event_monome_add monome_add;
    struct event_monome_remove monome_remove;
    struct event_grid_key grid_key;
    struct event_input_add input_add;
    struct event_input_remove input_remove;
    struct event_input_event input_event;
    struct event_timer timer;
    struct event_poll_value poll_value;
    struct event_poll_data poll_data;
};
