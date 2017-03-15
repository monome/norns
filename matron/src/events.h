#pragma once

typedef enum {
  // code to be executed by luavm
  EVENT_EXEC_CODE_LINE,
  // monome grid press
  EVENT_GRID_PRESS,
  // monome grid lift
  EVENT_GRID_LIFT,
  // timer has fired
  EVENT_TIMER,
  // finished receiving audio engine list
  EVENT_ENGINE_REPORT,
  // finished receiving command list
  EVENT_COMMAND_REPORT,
  // libmonome device added
  EVENT_MONOME_ADD,
  // libmonome device removed
  EVENT_MONOME_REMOVE,
  // quit the event loop
  EVENT_QUIT
} event_t;

extern void events_init(void);
extern void event_loop(void);
extern void event_post(event_t evcode, void* data1, void* data2);
// FIXME: this shouldn't really have to be a separate thing
extern void event_post_grid_event(event_t evcode, void* md, int x, int y);
