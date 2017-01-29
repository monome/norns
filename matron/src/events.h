#pragma once

typedef enum {
  // code to be executed by luavm
  EVENT_EXEC_CODE,
  // monome grid press
  EVENT_GRID_PRESS,
  // monome grid lift
  EVENT_GRID_LIFT,
  // timer has fired
  EVENT_TIMER,
  // finished receiving buffer list
  EVENT_BUFFER_REPORT,
  // finish receiving audio engine list
  EVENT_ENGINE_REPORT,
  // finished receiveing parameter list
  EVENT_PARAM_REPORT,
  // quit the event loop
  EVENT_QUIT
} event_t;

extern void events_init(void);
extern void event_loop(void);
extern void event_post(event_t ev, void* data1, void* data2);
