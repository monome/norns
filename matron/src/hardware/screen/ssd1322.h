#pragma once

#include <arm_neon.h>
#include <cairo.h>
#include <fcntl.h>
#include <gpiod.h>
#include <pthread.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "platform.h"

#define SPIDEV_0_0_PATH "/dev/spidev0.0"
#define SPI0_BUS_WIDTH 8

// Pinout
// see: https://github.com/monome/norns-image/blob/main/readme-hardware.md#pinout-1
#define SSD1322_DC_AND_RESET_GPIO_CHIP "gpiochip0"
#define SSD1322_DC_GPIO_LINE 5
#define SSD1322_RESET_GPIO_LINE 6

// Commands
// see:
#define SSD1322_ENABLE_GRAYSCALE_TABLE        0x00
#define SSD1322_SET_COLUMN_ADDRESS            0x15
#define SSD1322_WRITE_RAM_COMMAND             0x5C
#define SSD1322_READ_RAM_COMMAND              0x5D
#define SSD1322_SET_ROW_ADDRESS               0x75
#define SSD1322_SET_DUAL_COMM_LINE_MODE       0xA0
#define SSD1322_SET_DISPLAY_START_LINE        0xA1
#define SSD1322_SET_DISPLAY_OFFSET            0xA2
#define SSD1322_SET_DISPLAY_MODE_ALL_OFF      0xA4
#define SSD1322_SET_DISPLAY_MODE_ALL_ON       0xA5
#define SSD1322_SET_DISPLAY_MODE_NORMAL       0xA6 // listed 0xA4 in reference.
#define SSD1322_SET_DISPLAY_MODE_INVERSE      0xA7
#define SSD1322_ENABLE_PARTIAL_DISPLAY        0xA8
#define SSD1322_EXIT_PARTIAL_DISPLAY          0xA9
#define SSD1322_SET_VDD_REGULATOR             0xAB
#define SSD1322_SET_DISPLAY_OFF               0xAE
#define SSD1322_SET_DISPLAY_ON                0xAF
#define SSD1322_SET_PHASE_LENGTH              0xB1
#define SSD1322_SET_OSCILLATOR_FREQUENCY      0xB3
#define SSD1322_SET_DISPLAY_ENHANCEMENT_A     0xB4
#define SSD1322_SET_GPIO                      0xB5
#define SSD1322_SET_SECOND_PRECHARGE_PERIOD   0xB6
#define SSD1322_SET_GRAYSCALE_TABLE           0xB8
#define SSD1322_SET_DEFAULT_LINEAR_GRAY_SCALE 0xB9
#define SSD1322_SET_PRECHARGE_VOLTAGE         0xBB
#define SSD1322_SET_VCOMH_VOLTAGE             0xBE
#define SSD1322_SET_CONTRAST_CURRENT          0xC1
#define SSD1322_MASTER_CURRENT_CONTROL        0xC7
#define SSD1322_SET_MULTIPLEX_RATIO           0xCA
#define SSD1322_SET_DISPLAY_ENHANCEMENT_B     0xD1
#define SSD1322_SET_COMMAND_LOCK              0xFD

#define NORNS_PHASE_1_LENGTH 0x02 // Copied from fbtft-ssd1322.c
#define NORNS_PHASE_2_LENGTH 0xF0 // Copied from fbtft-ssd1322.c
#define NORNS_PHASE_LENGTH (NORNS_PHASE_1_LENGTH | NORNS_PHASE_2_LENGTH)
#define SSD1322_PHASE_LENGTH_RESET 0x74 // (SSD1322 rev 1.2, P 33/60)
#define SSD1322_PHASE_1_LENGTH_FROM_HEX(h) ( (h*2) + 1 )
#define SSD1322_PHASE_2_LENGTH_FROM_HEX(h) ( h >> 4 )

#define NORNS_MUX_RATIO 0x3F // Copied from fbtft-ssd1322.c
#define SSD1322_MUX_RATIO_FROM_HEX(h) ( h + 1 )

#define SSD1322_GRAYSCALE_MAX_VALUE 112.0

#define SSD1322_PIXEL_WIDTH 128
#define SSD1322_PIXEL_HEIGHT 64

typedef enum {
    SSD1322_DISPLAY_MODE_ALL_OFF = 0,
    SSD1322_DISPLAY_MODE_ALL_ON,
    SSD1322_DISPLAY_MODE_NORMAL,
    SSD1322_DISPLAY_MODE_INVERT,
} ssd1322_display_mode_t;

void ssd1322_init();
void ssd1322_deinit();
void ssd1322_refresh();
void ssd1322_update(cairo_surface_t * surface, bool should_translate_color);
void ssd1322_set_brightness(uint8_t b);
void ssd1322_set_contrast(uint8_t c);
void ssd1322_set_display_mode(ssd1322_display_mode_t);
void ssd1322_set_gamma(double g);
void ssd1322_set_refresh_rate(uint8_t hz);
uint8_t* ssd1322_resize_buffer(size_t);
