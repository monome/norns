// screen events: private data types
// (separate from screen_events.c only for readability)

#ifndef _SCREEN_EVENTS_PR_H_
#define _SCREEN_EVENTS_PR_H_

typedef enum {
	      SCREEN_EVENT_NONE = 0, // unused / init
	      SCREEN_EVENT_UPDATE = 1,
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
	      SCREEN_EVENT_TEXT_RIGHT,
	      SCREEN_EVENT_TEXT_CENTER,
	      SCREEN_EVENT_TEXT_EXTENTS,
	      SCREEN_EVENT_TEXT_TRIM,
	      SCREEN_EVENT_CLEAR,
	      SCREEN_EVENT_CLOSE_PATH,
	      SCREEN_EVENT_EXPORT_PNG,
	      SCREEN_EVENT_DISPLAY_PNG,
	      SCREEN_EVENT_ROTATE,
	      SCREEN_EVENT_TRANSLATE,
	      SCREEN_EVENT_SET_OPERATOR,
	      SCREEN_EVENT_PEEK,
	      SCREEN_EVENT_POKE,
	      SCREEN_EVENT_CURRENT_POINT,
	      SCREEN_EVENT_GAMMA,
	      SCREEN_EVENT_BRIGHTNESS,
	      SCREEN_EVENT_CONTRAST,
	      SCREEN_EVENT_INVERT,
	      
} screen_event_id_t;

//----------------
struct se_int {
    int i1;
};

struct se_doubles {
    double d1;
    double d2;
    double d3;
    double d4;
    double d5;
    double d6;
};

// string and binary types use the same buffer
struct se_buf {
    size_t nb;
};
    
struct se_buf_doubles {
    size_t nb;
    double d1;
    double d2;
    double d3;
    double d4;
};

struct se_buf_ints {
    size_t nb;
    int i1;
    int i2;
    int i3;
    int i4;
};
union screen_event_payload {
    struct se_int i;
    struct se_doubles d;
    struct se_buf b;
    struct se_buf_doubles bd;
    struct se_buf_ints bi;
};

struct screen_event_data {
    int type;
    void *buf;
    union screen_event_payload payload;
};

#endif
