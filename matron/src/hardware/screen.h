#pragma once

#include <stdint.h>

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
extern void screen_curve(double x1, double y1, double x2, double y2, double x3, double y3);
extern void screen_curve_rel(double dx1, double dy1, double dx2, double dy2, double dx3, double dy3);
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
