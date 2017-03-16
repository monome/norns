#pragma once

#include <stdint.h>

typedef enum {
  // unused (do not remove)
  EVENT_FIRST_EVENT = 0,
  // code to be executed by luavm
  EVENT_EXEC_CODE_LINE = 1,
  // timer has fired
  EVENT_TIMER = 2,
  // libmonome device added
  EVENT_MONOME_ADD = 3,
  // libmonome device removed
  EVENT_MONOME_REMOVE = 4,
  // monome grid press/lift
  EVENT_GRID_KEY = 5,
  // libevdev device added
  EVENT_INPUT_ADD = 6,
  // libevdev device removed
  EVENT_INPUT_REMOVE = 7,
  // input gesture
  EVENT_INPUT_EVENT = 8,
  // finished receiving audio engine list
  EVENT_ENGINE_REPORT = 9,
  // finished receiving command list
  EVENT_COMMAND_REPORT = 10,
  // quit the event loop
  EVENT_QUIT = 11,
} event_t;

struct event_common {
  uint32_t type;
  // could put timestamp here if we want
}; // +4

struct event_exec_code_line {
  struct event_common common;
  char* line;
}; // +4

struct event_monome_add {
  struct event_common common;
  void* dev;
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
  void* dev;
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
  uint8_t code;
  uint8_t pad;
  int32_t value;
}; // +8

struct event_timer {
  struct event_common common;
  uint32_t id;
  uint32_t stage;
}; // +4

union event_data {
  uint32_t type;
  struct event_exec_code_line exec_code_line ;
  struct event_monome_add monome_add ;
  struct event_monome_remove monome_remove ;
  struct event_grid_key grid_key ;
  struct event_input_add input_add ;
  struct event_input_remove input_remove ;
  struct event_input_event input_event ;
  struct event_timer timer ;
};
