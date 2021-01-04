#ifndef _SCREEN_EVENTS_H_
#define _SCREEN_EVENTS_H_

#include <stdarg.h>

typedef struct { 
    int type;
    int data_count;
    double data[6];
    char* text;
} screen_event_data_t;

typedef enum {
  SCREEN_EVENT_UPDATE = 0,
  SCREEN_EVENT_SAVE,
  SCREEN_EVENT_RESTORE,
  SCREEN_EVENT_FONT_FACE,
  SCREEN_EVENT_FONT_SIZE,
  SCREEN_EVENT_AA,
  SCREEN_EVENT_LEVEL,
  SCREEN_EVENT_LINE_WIDTH,
  SCREEN_EVENT_LINE_CAP,
  SCREEN_EVENT_LINE_JOIN,
  SCREEN_EVENT_MITER_LIMIT,
  SCREEN_EVENT_MOVE,
  SCREEN_EVENT_LINE,
  SCREEN_EVENT_MOVE_REL,
  SCREEN_EVENT_LINE_REL,
  SCREEN_EVENT_CURVE,
  SCREEN_EVENT_CURVE_REL,
  SCREEN_EVENT_ARC,
  SCREEN_EVENT_RECT,
  SCREEN_EVENT_STROKE,
  SCREEN_EVENT_FILL,
  SCREEN_EVENT_TEXT,
  SCREEN_EVENT_CLEAR,
  SCREEN_EVENT_CLOSE_PATH,
  SCREEN_EVENT_EXTENTS,
  SCREEN_EVENT_EXPORT_PNG,
  SCREEN_EVENT_DISPLAY_PNG,
  SCREEN_ROTATE,
  SCREEN_TRANSLATE,
  SCREEN_SET_OPERATOR
} screen_event_id_t;

extern void screen_events_init();

extern void screen_event_push(screen_event_id_t id, const char* text, int arg_count, ...);


#endif