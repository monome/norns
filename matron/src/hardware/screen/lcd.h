#ifndef __LCD_H
#define __LCD_H

#include <arm_neon.h>
#include <cairo.h>
#include <gpiod.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/spi/spidev.h>

#define SPIDEV_0_0_PATH "/dev/spidev0.0"
#define SPI0_BUS_WIDTH 8

// GPIO Configuration
#define LCD_DC_AND_RESET_GPIO_CHIP "gpiochip0"
#define LCD_DC_GPIO_LINE 25  // DC pin
#define LCD_RESET_GPIO_LINE 27  // RST pin
#define LCD_BL_GPIO_LINE 18  // Backlight pin
#define LCD_CS_GPIO_LINE 8   // Chip Select pin

// Display dimensions
#define LCD_WIDTH 240
#define LCD_HEIGHT 135

// Display commands
#define LCD_SWRESET 0x01
#define LCD_SLPIN 0x10
#define LCD_SLPOUT 0x11
#define LCD_DISPOFF 0x28
#define LCD_DISPON 0x29
#define LCD_CASET 0x2A
#define LCD_RASET 0x2B
#define LCD_RAMWR 0x2C
#define LCD_MADCTL 0x36
#define LCD_COLMOD 0x3A
#define LCD_FRMCTR1 0xB1
#define LCD_FRMCTR2 0xB2
#define LCD_FRMCTR3 0xB3
#define LCD_INVCTR 0xB4
#define LCD_DISSET5 0xB6
#define LCD_PWCTR1 0xC0
#define LCD_PWCTR2 0xC1
#define LCD_PWCTR3 0xC2
#define LCD_PWCTR4 0xC3
#define LCD_PWCTR5 0xC4
#define LCD_VMCTR1 0xC5
#define LCD_RDID1 0xDA
#define LCD_RDID2 0xDB
#define LCD_RDID3 0xDC
#define LCD_RDID4 0xDD
#define LCD_GMCTRP1 0xE0
#define LCD_GMCTRN1 0xE1
#define LCD_PWCTR6 0xFC

// Display modes
typedef enum {
    LCD_DISPLAY_MODE_NORMAL = 0,
    LCD_DISPLAY_MODE_INVERT = 1,
    LCD_DISPLAY_MODE_OFF = 2,
    LCD_DISPLAY_MODE_ON = 3
} lcd_display_mode_t;

// Function declarations
void lcd_init(void);
void lcd_deinit(void);
void lcd_refresh(void);
void lcd_update(cairo_surface_t * surface_pointer, bool surface_may_have_color);
void lcd_set_brightness(uint8_t b);
void lcd_set_contrast(uint8_t c);
void lcd_set_display_mode(lcd_display_mode_t mode);
void lcd_set_gamma(double g);
void lcd_set_refresh_rate(uint8_t hz);
uint8_t* lcd_resize_buffer(size_t size);

#endif 