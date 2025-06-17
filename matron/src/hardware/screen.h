#ifndef SCREEN_H
#define SCREEN_H

#include <cairo.h>

// Initialize the display
int screen_init(void);

// Deinitialize the display
void screen_deinit(void);

// Update the display with new surface data
void screen_update(cairo_surface_t *surface);

// Set display brightness (0.0-1.0)
void screen_brightness(float v);

// Set display contrast (0.0-1.0)
void screen_contrast(float c);

// Set display mode (0=normal, 1=inverted)
void screen_invert(int inverted);

// Set gamma correction
void screen_gamma(float g);

// Set refresh rate in Hz
void screen_refresh_rate(uint8_t rate);

#endif // SCREEN_H
