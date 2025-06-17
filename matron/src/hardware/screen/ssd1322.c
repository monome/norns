#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <gpiod.h>
#include <pthread.h>
#include <arm_neon.h>
#include <cairo.h>
#include <cairo-ft.h>
#include "ssd1322.h"
#include "lcd.h"

// Buffer for surface data
static uint32_t *surface_buffer = NULL;
static pthread_mutex_t surface_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread for refreshing the display
static pthread_t refresh_thread;
static int refresh_thread_running = 0;

// Function to run in the refresh thread
static void *ssd1322_thread_run(void *arg) {
    (void)arg;
    while (refresh_thread_running) {
        lcd_refresh();
        usleep(16667); // ~60Hz refresh rate
    }
    return NULL;
}

// Initialize the display
int ssd1322_init(void) {
    // Initialize the LCD
    if (lcd_init() != 0) {
        return -1;
    }

    // Create surface buffer
    surface_buffer = calloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint32_t), 1);
    if (!surface_buffer) {
        return -1;
    }

    // Start refresh thread
    refresh_thread_running = 1;
    if (pthread_create(&refresh_thread, NULL, ssd1322_thread_run, NULL) != 0) {
        free(surface_buffer);
        return -1;
    }

    return 0;
}

// Deinitialize the display
void ssd1322_deinit(void) {
    // Stop refresh thread
    refresh_thread_running = 0;
    pthread_join(refresh_thread, NULL);

    // Free surface buffer
    if (surface_buffer) {
        free(surface_buffer);
        surface_buffer = NULL;
    }

    // Deinitialize LCD
    lcd_deinit();
}

// Update the display with new surface data
void ssd1322_update(cairo_surface_t *surface) {
    if (!surface_buffer) return;

    pthread_mutex_lock(&surface_mutex);
    memcpy(surface_buffer, cairo_image_surface_get_data(surface),
           LCD_WIDTH * LCD_HEIGHT * sizeof(uint32_t));
    pthread_mutex_unlock(&surface_mutex);
}

// Set display brightness
void ssd1322_set_brightness(uint8_t b) {
    lcd_set_brightness(b);
}

// Set display contrast
void ssd1322_set_contrast(uint8_t c) {
    lcd_set_contrast(c);
}

// Set display mode
void ssd1322_set_display_mode(lcd_display_mode_t mode) {
    lcd_set_display_mode(mode);
}

// Set gamma correction
void ssd1322_set_gamma(double gamma) {
    lcd_set_gamma(gamma);
}

// Set refresh rate
void ssd1322_set_refresh_rate(uint8_t rate) {
    lcd_set_refresh_rate(rate);
}
