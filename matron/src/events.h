#pragma once

typedef enum {
  EVENT_GRID_PRESS,
  EVENT_TIMER,
  EVENT_QUIT
} event_t;

extern void events_init(void);
extern void event_loop(void);
extern void event_post(event_t ev, void* data1, void* data2);
