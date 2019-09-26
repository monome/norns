#pragma once

#include <stdint.h>
#include "oracle.h"
#include "osc.h"

typedef enum {
    // unused (do not remove)
    EVENT_FIRST_EVENT = 0,
    // code to be executed by luavm
    EVENT_EXEC_CODE_LINE,
    // metro has fired
    EVENT_METRO,
    // clock resume requested
    EVENT_CLOCK_RESUME,
    // gpio event
    EVENT_KEY,
    // gpio event
    EVENT_ENC,
    // battery level change
    EVENT_BATTERY,
    // power cable present
    EVENT_POWER,
    // stat update (disk, temp, cpu)
    EVENT_STAT,
    // libmonome device added
    EVENT_MONOME_ADD,
    // libmonome device removed
    EVENT_MONOME_REMOVE,
    // monome grid press/lift
    EVENT_GRID_KEY,
    // monome arc encoder delta
    EVENT_ARC_ENCODER_DELTA,
    // monome arc encoder key
    EVENT_ARC_ENCODER_KEY,
    // libevdev device added
    EVENT_HID_ADD,
    // libevdev device removed
    EVENT_HID_REMOVE,
    // hid gesture
    EVENT_HID_EVENT,
    // midi device added
    EVENT_MIDI_ADD,
    // midi device removed
    EVENT_MIDI_REMOVE,
    // midi event
    EVENT_MIDI_EVENT,
    // incoming OSC event
    EVENT_OSC,
    // finished receiving audio engine list
    EVENT_ENGINE_REPORT,
    // finished loading engine
    EVENT_ENGINE_LOADED,
    // polled value from crone
    EVENT_POLL_VALUE,
    // arbitrary polled data from crone
    EVENT_POLL_DATA,
    // polled waveform from crone
    EVENT_POLL_WAVE,
    // polled i/o VU levels from crone
    EVENT_POLL_IO_LEVELS,
    // polled softcut phase
    EVENT_POLL_SOFTCUT_PHASE,
    // crone startup ack event
    EVENT_STARTUP_READY_OK,
    // crone startup timeout event
    EVENT_STARTUP_READY_TIMEOUT,
    // system command finished
    EVENT_SYSTEM_CMD,
    // reset the lua state
    EVENT_RESET_LVM,
    // quit the event loop
    EVENT_QUIT,
    // crow add
    EVENT_CROW_ADD,
    // crow remove
    EVENT_CROW_REMOVE,
    // crow event
    EVENT_CROW_EVENT
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

struct event_arc_encoder_delta {
    struct event_common common;
    uint8_t id;
    uint8_t number;
    int8_t delta;
}; // +4

struct event_arc_encoder_key {
    struct event_common common;
    uint8_t id;
    uint8_t number;
    int8_t state;
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

struct event_midi_add {
    struct event_common common;
    void *dev;
}; // +4

struct event_midi_remove {
    struct event_common common;
    uint32_t id;
}; // +4

struct event_midi_event {
    struct event_common common;
    uint32_t id;
    uint8_t data[3];
    size_t nbytes;
}; // +11

struct event_osc {
    struct event_common common;
    char *path;
    char *from_host;
    char *from_port;
    lo_message msg;
}; // +16?

struct event_metro {
    struct event_common common;
    uint32_t id;
    uint32_t stage;
}; // +8

struct event_clock_resume {
    struct event_common common;
    uint32_t thread_id;
};

struct event_key {
    struct event_common common;
    uint8_t n;
    uint8_t val;
}; // +2

struct event_battery {
    struct event_common common;
    uint8_t percent;
    int16_t current;
}; // +3

struct event_power {
    struct event_common common;
    uint8_t present;
}; // +1

struct event_stat {
    struct event_common common;
    uint16_t disk;
    uint8_t temp;
    uint8_t cpu;
};

struct event_enc {
    struct event_common common;
    uint8_t n;
    int8_t delta;
}; // +2

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

struct event_poll_softcut_phase {
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

// like DATA, but size is fixed
struct event_poll_wave {
    struct event_common common;
    uint32_t idx;
    uint8_t *data;
}; // + 8

struct event_startup_ready_ok {
    struct event_common common;
}; // + 0

struct event_startup_ready_timeout {
    struct event_common common;
}; // + 0

struct event_crow_add {
    struct event_common common;
    void *dev;
}; // +4

struct event_crow_remove {
    struct event_common common;
    uint32_t id;
}; // +4

struct event_crow_event {
    struct event_common common;
		void *dev;
    uint8_t id;
}; // +4

struct event_system_cmd {
    struct event_common common;
    char *capture;
};


union event_data {
    uint32_t type;
    struct event_exec_code_line exec_code_line;
    struct event_monome_add monome_add;
    struct event_monome_remove monome_remove;
    struct event_grid_key grid_key;
    struct event_arc_encoder_delta arc_encoder_delta;
    struct event_arc_encoder_key arc_encoder_key;
    struct event_hid_add hid_add;
    struct event_hid_remove hid_remove;
    struct event_hid_event hid_event;
    struct event_midi_add midi_add;
    struct event_midi_remove midi_remove;
    struct event_midi_event midi_event;
    struct event_osc osc_event;
    struct event_key key;
    struct event_enc enc;
    struct event_battery battery;
    struct event_power power;
    struct event_stat stat;
    struct event_metro metro;
    struct event_clock_resume clock_resume;
    struct event_poll_value poll_value;
    struct event_poll_data poll_data;
    struct event_poll_io_levels poll_io_levels;
    struct event_poll_softcut_phase softcut_phase;
    struct event_poll_wave poll_wave;
    struct event_startup_ready_ok startup_ready_ok;
    struct event_startup_ready_timeout startup_ready_timeout;
    struct event_crow_add crow_add;
    struct event_crow_remove crow_remove;
    struct event_crow_event crow_event;
    struct event_system_cmd system_cmd;
};
