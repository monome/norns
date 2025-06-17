#include "lcd.h"

#include <string.h>

#include "events.h"
#include "event_types.h"

static int spidev_fd = 0;
static bool display_dirty = false;
static bool should_translate_color = true; // LCD supports color
static bool should_turn_on = true;
static uint8_t * spidev_buffer = NULL;
static uint32_t * surface_buffer = NULL;
static struct gpiod_chip * gpio_0;
static struct gpiod_line * gpio_dc;
static struct gpiod_line * gpio_reset;
static struct gpiod_line * gpio_bl;
static struct gpiod_line * gpio_cs;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t lcd_pthread_t;

#define SPIDEV_BUFFER_LEN  LCD_WIDTH * LCD_HEIGHT * 2 // 16-bit color
#define SURFACE_BUFFER_LEN LCD_WIDTH * LCD_HEIGHT * sizeof(uint32_t)

int open_spi() {
    uint8_t mode = SPI_MODE_0;
    uint8_t bits_per_word = SPI0_BUS_WIDTH;
    uint8_t little_endian = 0;
    uint32_t speed_hz = 1200000000 / 64; // 18.75Mhz

    int fd = open(SPIDEV_0_0_PATH, O_RDWR | O_SYNC);

    if( fd < 0 ){
        fprintf(stderr, "(screen) couldn't open %s\n", SPIDEV_0_0_PATH);
        return -1;
    }

    int outcome = 0
    || ( ioctl(fd, SPI_IOC_WR_MODE, &mode)                   < 0 )
    || ( ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0 )
    || ( ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz)       < 0 )
    || ( ioctl(fd, SPI_IOC_WR_LSB_FIRST, &little_endian)     < 0 );
    if( outcome != 0 ){
       fprintf(stderr, "could not set SPI WR settings via IOC\n");
       close(fd);
       return -1;
    }

    return fd;
}

int lcd_write_command(uint8_t command, uint8_t data_len, ...) {
    va_list args;
    uint8_t cmd_buf[1];
    uint8_t data_buf[256];
    struct spi_ioc_transfer cmd_transfer = {0};
    struct spi_ioc_transfer data_transfer = {0};

    pthread_mutex_lock(&lock);

    if( spidev_fd <= 0 ){
        fprintf(stderr, "%s: spidev not yet opened\n", __func__);
        goto fail;
    }

    // Set CS low to start transaction
    gpiod_line_set_value(gpio_cs, 0);
    
    // Set DC low for command
    gpiod_line_set_value(gpio_dc, 0);

    cmd_buf[0] = command;
    cmd_transfer.tx_buf = (unsigned long) cmd_buf;
    cmd_transfer.len = (uint32_t) sizeof(cmd_buf);

    if( ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &cmd_transfer) < 0 ){
        fprintf(stderr, "%s: could not send command-message.\n", __func__);
        goto fail;
    }

    if( data_len > 0 ){
        // Set DC high for data
        gpiod_line_set_value(gpio_dc, 1);

        va_start(args, data_len);

        for( uint8_t i = 0; i < data_len; i++ ){
            data_buf[i] = va_arg(args, int);
        }

        va_end(args);

        data_transfer.tx_buf = (unsigned long) data_buf;
        data_transfer.len = (uint32_t) data_len;

        if( ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &data_transfer) < 0 ){
            fprintf(stderr, "%s: could not send data-message.\n", __func__);
            goto fail;
        }
    }

    // Set CS high to end transaction
    gpiod_line_set_value(gpio_cs, 1);

    pthread_mutex_unlock(&lock);
    return 0;
fail:
    // Ensure CS is high on error
    gpiod_line_set_value(gpio_cs, 1);
    pthread_mutex_unlock(&lock);
    return -1;
}

#ifndef NUMARGS
#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__}) / sizeof(int))
#endif
#define write_command_with_data(x, ...) \
    (lcd_write_command(x, NUMARGS(__VA_ARGS__), __VA_ARGS__))
#define write_command(x) \
    (lcd_write_command(x, 0, 0))

static void* lcd_thread_run(void * p){
    (void)p;

    static struct timespec ts = {
            .tv_sec = 0,
            .tv_nsec = 16666666, // 60Hz
    };

    while( spidev_buffer ){
        if( display_dirty ){
            lcd_refresh();
            display_dirty = false;
        }

        event_post(event_data_new(EVENT_SCREEN_REFRESH));

        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
    }

    return NULL;
}

void lcd_init() {
    if( pthread_mutex_init(&lock, NULL) != 0 ){
        fprintf(stderr, "%s: pthread_mutex_init failed\n", __func__);
        return;
    }

    surface_buffer = calloc(SURFACE_BUFFER_LEN, 1);
    if( surface_buffer == NULL ){
        fprintf(stderr, "%s: couldn't allocate surface_buffer\n", __func__);
        return;
    }

    spidev_buffer = calloc(SPIDEV_BUFFER_LEN, 1);
    if( spidev_buffer == NULL ){
        fprintf(stderr, "%s: couldn't allocate spidev_buffer\n", __func__);
        return;
    }

    spidev_fd = open_spi(SPIDEV_0_0_PATH);
    if( spidev_fd < 0 ){
        fprintf(stderr, "%s: couldn't open %s.\n", __func__, SPIDEV_0_0_PATH);
        return;
    }

    gpio_0 = gpiod_chip_open_by_name(LCD_DC_AND_RESET_GPIO_CHIP);
    gpio_dc = gpiod_chip_get_line(gpio_0, LCD_DC_GPIO_LINE);
    gpio_reset = gpiod_chip_get_line(gpio_0, LCD_RESET_GPIO_LINE);
    gpio_bl = gpiod_chip_get_line(gpio_0, LCD_BL_GPIO_LINE);
    gpio_cs = gpiod_chip_get_line(gpio_0, LCD_CS_GPIO_LINE);

    gpiod_line_request_output(gpio_dc, "D/C", 0);
    gpiod_line_request_output(gpio_reset, "RST", 0);
    gpiod_line_request_output(gpio_bl, "BL", 0);
    gpiod_line_request_output(gpio_cs, "CS", 1); // CS is active low

    // Reset sequence
    gpiod_line_set_value(gpio_reset, 1);
    usleep(100000);
    gpiod_line_set_value(gpio_reset, 0);
    usleep(100000);
    gpiod_line_set_value(gpio_reset, 1);
    usleep(100000);

    // Initialize display
    write_command(LCD_SWRESET);
    usleep(120000);

    write_command_with_data(LCD_PWCTR1, 0x23);
    write_command_with_data(LCD_PWCTR2, 0x10);
    write_command_with_data(LCD_VMCTR1, 0x3E, 0x28);
    write_command_with_data(LCD_FRMCTR1, 0x00, 0x18);
    write_command_with_data(LCD_COLMOD, 0x05); // 16-bit color
    write_command_with_data(LCD_MADCTL, 0x48); // Row/Column order
    write_command_with_data(LCD_GMCTRP1, 0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20, 0x22, 0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10);
    write_command_with_data(LCD_GMCTRN1, 0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29, 0x2E, 0x30, 0x30, 0x39, 0x3F, 0x00, 0x07, 0x03, 0x10);

    write_command(LCD_SLPOUT);
    usleep(120000);
    write_command(LCD_DISPON);

    // Turn on backlight
    gpiod_line_set_value(gpio_bl, 1);

    // Set high thread priority
    static struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_OTHER);

    // Start thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&lcd_pthread_t, &attr, &lcd_thread_run, NULL);
    pthread_attr_destroy(&attr);
}

void lcd_deinit(){
    if( spidev_fd > 0 ){
        // Turn off backlight
        gpiod_line_set_value(gpio_bl, 0);
        
        // Turn off display
        write_command(LCD_DISPOFF);
        write_command(LCD_SLPIN);

        // Drive RST low
        gpiod_line_set_value(gpio_reset, 0);

        // Cleanup
        pthread_mutex_destroy(&lock);
        gpiod_line_release(gpio_reset);
        gpiod_line_release(gpio_dc);
        gpiod_line_release(gpio_bl);
        gpiod_line_release(gpio_cs);
        gpiod_chip_close(gpio_0);
        close(spidev_fd);

        free(spidev_buffer);
        spidev_buffer = NULL;
    }
}

void lcd_update(cairo_surface_t * surface_pointer, bool surface_may_have_color){
    pthread_mutex_lock(&lock);

    should_translate_color = surface_may_have_color;

    if( surface_buffer != NULL && surface_pointer != NULL ){
        const uint32_t surface_w = cairo_image_surface_get_width(surface_pointer);
        const uint32_t surface_h = cairo_image_surface_get_height(surface_pointer);
        cairo_format_t surface_f = cairo_image_surface_get_format(surface_pointer);

        if( surface_w != LCD_WIDTH || surface_h != LCD_HEIGHT || surface_f != CAIRO_FORMAT_ARGB32 ){
            fprintf(stderr, "%s: %ux%u = invalid surface size\n", __func__, surface_w, surface_h);
            goto early_return;
        }

        memcpy(
            (uint8_t *) surface_buffer,
            (uint8_t *) cairo_image_surface_get_data(surface_pointer),
            SURFACE_BUFFER_LEN
        );
    }
    else{
        fprintf(stderr, "%s: surface_buffer (%p) surface_pointer (%p)\n", __func__, surface_buffer, surface_pointer);
    }

    display_dirty = true;

early_return:
    pthread_mutex_unlock(&lock);
}

void lcd_refresh() {
    struct spi_ioc_transfer transfer = {0};

    if( spidev_fd <= 0 ){
        fprintf(stderr, "%s: spidev not yet opened.\n", __func__);
        return;
    }

    if( surface_buffer == NULL ){
        fprintf(stderr, "%s: surface_buffer not allocated yet.\n", __func__);
        return;
    }

    // Set column and row addresses
    write_command_with_data(LCD_CASET, 0, 0, (LCD_WIDTH >> 8) & 0xFF, LCD_WIDTH & 0xFF);
    write_command_with_data(LCD_RASET, 0, 0, (LCD_HEIGHT >> 8) & 0xFF, LCD_HEIGHT & 0xFF);
    write_command(LCD_RAMWR);

    pthread_mutex_lock(&lock);

    // Convert ARGB32 to RGB565
    for( uint32_t i = 0; i < SPIDEV_BUFFER_LEN; i += 2 ){
        uint32_t pixel = surface_buffer[i/2];
        uint8_t r = (pixel >> 16) & 0xFF;
        uint8_t g = (pixel >> 8) & 0xFF;
        uint8_t b = pixel & 0xFF;
        
        // Convert to RGB565
        uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        
        spidev_buffer[i] = rgb565 >> 8;
        spidev_buffer[i+1] = rgb565 & 0xFF;
    }

    // Set CS low to start transaction
    gpiod_line_set_value(gpio_cs, 0);
    
    // Set DC high for data
    gpiod_line_set_value(gpio_dc, 1);

    const uint32_t spidev_bufsize = 8192; // Max is defined in /boot/config.txt
    const uint32_t n_transfers = SPIDEV_BUFFER_LEN / spidev_bufsize;
    for( uint32_t i = 0; i < n_transfers; i++ ){
        transfer.tx_buf = (unsigned long) (spidev_buffer + (i * spidev_bufsize));
        transfer.len = SPIDEV_BUFFER_LEN / n_transfers;
        if( ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &transfer) < 0 ){
            fprintf(stderr, "%s: SPI data transfer %d of %d failed.\n",
                            __func__,               i,    n_transfers);
            goto early_return;
        }
    }

    // Set CS high to end transaction
    gpiod_line_set_value(gpio_cs, 1);

early_return:
    pthread_mutex_unlock(&lock);
    return;
}

void lcd_set_brightness(uint8_t b){
    // Control backlight PWM
    gpiod_line_set_value(gpio_bl, b > 0 ? 1 : 0);
}

void lcd_set_contrast(uint8_t c){
    write_command_with_data(LCD_PWCTR1, c);
}

void lcd_set_display_mode(lcd_display_mode_t mode){
    switch(mode) {
        case LCD_DISPLAY_MODE_NORMAL:
            write_command_with_data(LCD_MADCTL, 0x48);
            break;
        case LCD_DISPLAY_MODE_INVERT:
            write_command_with_data(LCD_MADCTL, 0xC8);
            break;
        case LCD_DISPLAY_MODE_OFF:
            write_command(LCD_DISPOFF);
            break;
        case LCD_DISPLAY_MODE_ON:
            write_command(LCD_DISPON);
            break;
    }
}

void lcd_set_gamma(double g){
    // Gamma correction is handled by the display's internal gamma table
    // No need to implement software gamma correction
}

void lcd_set_refresh_rate(uint8_t hz){
    // Refresh rate is fixed by the display hardware
    // No need to implement refresh rate control
}

uint8_t* lcd_resize_buffer(size_t size){
    spidev_buffer = realloc(spidev_buffer, size);
    return spidev_buffer;
}

#undef NUMARGS
#undef write_command
#undef write_command_with_data
