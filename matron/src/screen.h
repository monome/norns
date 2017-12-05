#pragma once

#include <stdint.h>

extern void screen_init(void);
extern void screen_deinit(void);

extern void screen_pixel(int x, int y, int z);
extern void screen_line(int x1, int y1, int x2, int y2, int z);
extern void screen_text(int x, int y, int z, const char* s);
