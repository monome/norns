#pragma once

#include <stdint.h>

extern void screen_init(void);
extern void screen_deinit(void);

extern void screen_aa(int s);
extern void screen_level(int z);
extern void screen_line_width(long w);
extern void screen_move(int x, int y);
extern void screen_line(int x, int y);
extern void screen_stroke(void);
extern void screen_text(const char* s);
extern void screen_clear(void);
