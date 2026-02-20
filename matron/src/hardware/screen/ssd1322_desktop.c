#include "ssd1322.h"

void ssd1322_init(void) {
}

void ssd1322_deinit(void) {
}

void ssd1322_refresh(void) {
}

void ssd1322_update(cairo_surface_t *surface, bool translate) {
    (void)surface;
    (void)translate;
}

void ssd1322_set_brightness(uint8_t b) {
    (void)b;
}

void ssd1322_set_contrast(uint8_t c) {
    (void)c;
}

void ssd1322_set_display_mode(ssd1322_display_mode_t mode) {
    (void)mode;
}

void ssd1322_set_gamma(double g) {
    (void)g;
}

void ssd1322_set_refresh_rate(uint8_t hz) {
    (void)hz;
}

uint8_t *ssd1322_resize_buffer(size_t size) {
    (void)size;
    return NULL;
}
