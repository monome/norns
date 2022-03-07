#pragma once

#include <stdint.h>
#include <stdbool.h>

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
extern char *screen_peek(int x, int y, int *w, int *h);
extern void screen_poke(int x, int y, int w, int h, unsigned char *buf);
extern void screen_rotate(double r);
extern void screen_translate(double x, double y);
extern void screen_set_operator(int i);

typedef struct _screen_surface screen_surface_t;
typedef struct {
    int width;
    int height;
} screen_surface_extents_t;

extern screen_surface_t *screen_surface_new(double width, double height);
extern screen_surface_t *screen_surface_load_png(const char *filename);
extern void screen_surface_free(screen_surface_t *s);
extern bool screen_surface_get_extents(screen_surface_t *s, screen_surface_extents_t *e);
extern void screen_surface_display(screen_surface_t *s, double x, double y);
extern void screen_surface_display_region(screen_surface_t *s, double left, double top, double width, double height, double x, double y);

typedef struct _screen_context screen_context_t;

extern screen_context_t *screen_context_new(screen_surface_t *target);
extern void screen_context_free(screen_context_t *context);
extern const screen_context_t *screen_context_get_current(void);
extern void screen_context_set(const screen_context_t *context);
extern void screen_context_set_primary(void);

