#pragma once

#include <stdint.h>

extern union screen_event_data *screen_new_event();
extern void screen_post_event();

extern void screen_init(void);
extern void screen_deinit(void);

extern void screen_update(void);
extern void screen_save(void);
extern void screen_restore(void);
extern void screen_font_face(int i);
extern void screen_font_size(double z);
extern void screen_aa(int s);
extern void screen_level(int z);
extern void screen_line_width(double w);
extern void screen_line_cap(const char *style);
extern void screen_line_join(const char *style);
extern void screen_miter_limit(double limit);
extern void screen_move(double x, double y);
extern void screen_line(double x, double y);
extern void screen_move_rel(double x, double y);
extern void screen_line_rel(double x, double y);
extern void screen_curve(double x1,
                         double y1,
                         double x2,
                         double y2,
                         double x3,
                         double y3);
extern void screen_curve_rel(double dx1,
                             double dy1,
                             double dx2,
                             double dy2,
                             double dx3,
                             double dy3);
extern void screen_arc(double x, double y, double r, double a1, double a2);
extern void screen_rect(double x, double y, double w, double h);
extern void screen_stroke(void);
extern void screen_fill(void);
extern void screen_text(const char *s);
extern void screen_clear(void);
extern void screen_close_path(void);
extern double *screen_text_extents(const char *s);
extern void screen_export_png(const char *s);
extern void screen_display_png(const char *filename, double x, double y);

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
  SCREEN_EVENT_DISPLAY_PNG
} screen_event_t;


struct se_v {
  uint32_t type;
};

struct se_i {
  uint32_t type;
  int i1;
};

struct se_d {
  uint32_t type;
  double d1;
};

struct se_dd {
  uint32_t type;
  double d1;
  double d2;
};

struct se_ddd {
  uint32_t type;
  double d1;
  double d2;
  double d3;
};

struct se_dddd {
  uint32_t type;
  double d1;
  double d2;
  double d3;
  double d4;
};

struct se_ddddd {
  uint32_t type;
  double d1;
  double d2;
  double d3;
  double d4;
  double d5;
};

struct se_dddddd {
  uint32_t type;
  double d1;
  double d2;
  double d3;
  double d4;
  double d5;
  double d6;
};

struct se_c {
  uint32_t type;
  char *c1;
};

struct se_cdd {
  uint32_t type;
  char *c1;
  double d2;
  double d3;
};


union screen_event_data {
  uint32_t type;
  struct se_v v;
  struct se_i i;
  struct se_d d;
  struct se_dd dd;
  struct se_ddd ddd;
  struct se_dddd dddd;
  struct se_ddddd ddddd;
  struct se_dddddd dddddd;
  struct se_c c;
  struct se_cdd cdd;
};
