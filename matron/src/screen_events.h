#ifndef _SCREEN_EVENTS_H_
#define _SCREEN_EVENTS_H_

extern void screen_events_init();

extern void screen_event_update(void);
extern void screen_event_save(void);
extern void screen_event_restore(void);
extern void screen_event_font_face(int i);
extern void screen_event_font_size(double z);
extern void screen_event_aa(int s);
extern void screen_event_level(int z);
extern void screen_event_line_width(double w);
extern void screen_event_line_cap(const char *style);
extern void screen_event_line_join(const char *style);
extern void screen_event_miter_limit(double limit);
extern void screen_event_move(double x, double y);
extern void screen_event_line(double x, double y);
extern void screen_event_move_rel(double x, double y);
extern void screen_event_line_rel(double x, double y);
extern void screen_event_curve(double x1, double y1, double x2, double y2, double x3, double y3);
extern void screen_event_curve_rel(double dx1, double dy1, double dx2, double dy2, double dx3, double dy3);
extern void screen_event_arc(double x, double y, double r, double a1, double a2);
extern void screen_event_rect(double x, double y, double w, double h);
extern void screen_event_stroke(void);
extern void screen_event_fill(void);
extern void screen_event_text(const char *s);
extern void screen_event_clear(void);
extern void screen_event_close_path(void);
extern void screen_event_text_extents(const char *s);
extern void screen_event_export_png(const char *s);
extern void screen_event_display_png(const char *filename, double x, double y);
extern void screen_event_peek(int x, int y, int w, int h);
extern void screen_event_poke(int x, int y, int w, int h, unsigned char *buf);
extern void screen_event_rotate(double r);
extern void screen_event_translate(double x, double y);
extern void screen_event_set_operator(int i);
extern void screen_event_current_point();


#endif
