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
  /*   // finished receiving buffer list */
  /* EVENT_BUFFER_REPORT, */
  /* // finished receiving parameter list */
  /* EVENT_PARAM_REPORT, */
  // quit the event loop
  EVENT_QUIT
} event_t;

extern void events_init(void);
extern void event_loop(void);
extern void event_post(event_t id, void* data1, void* data2);
extern void event_post_monome_grid(event_t id, int x, int y);
