#pragma once

#include <stdint.h>

extern void screen_init(void);
extern void screen_deinit(void);

extern void screen_font_face(int i);
extern void screen_font_size(long z);
extern void screen_aa(int s);
extern void screen_level(int z);
extern void screen_line_width(long w);
extern void screen_move(long x, long y);
extern void screen_line(long x, long y);
extern void screen_move_rel(long x, long y);
extern void screen_line_rel(long x, long y);
extern void screen_curve(double x1, double y1, double x2, double y2, double x3, double y3);
extern void screen_curve_rel(double dx1, double dy1, double dx2, double dy2, double dx3, double dy3);
extern void screen_arc(double x, double y, double r, double a1, double a2);
extern void screen_rect(double x, double y, double w, double h);
extern void screen_stroke(void);
extern void screen_fill(void);
extern void screen_text(const char *s);
extern void screen_clear(void);
extern void screen_close_path(void); 
extern double * screen_extents(const char *s);
