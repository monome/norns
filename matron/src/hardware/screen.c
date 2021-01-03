/*
 * screen.c
 *
 * cairo drawing (and a little ioctl)
 */

#include <assert.h>
#include <cairo-ft.h>
#include <cairo.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "args.h"
#include "events.h"
#include "event_types.h"
#include "screen.h"

#define NUM_FONTS 67
#define NUM_OPS 29

#define USE_LOCK 0

#if USE_LOCK
#define LOCK_CR pthread_mutex_lock(&cr_lock);
#define UNLOCK_CR pthread_mutex_unlock(&cr_lock);
#else
#define LOCK_CR ;;
#define UNLOCK_CR ;;
#endif

static char font_path[NUM_FONTS][32];

static float c[16] = {0,   0.066666666666667, 0.13333333333333, 0.2, 0.26666666666667, 0.33333333333333,
                      0.4, 0.46666666666667,  0.53333333333333, 0.6, 0.66666666666667, 0.73333333333333,
                      0.8, 0.86666666666667,  0.93333333333333, 1};

static cairo_operator_t ops[NUM_OPS] = {
    CAIRO_OPERATOR_OVER,
    CAIRO_OPERATOR_XOR,
    CAIRO_OPERATOR_ADD,
    CAIRO_OPERATOR_SATURATE,
    CAIRO_OPERATOR_MULTIPLY,
    CAIRO_OPERATOR_SCREEN,
    CAIRO_OPERATOR_OVERLAY,
    CAIRO_OPERATOR_DARKEN,
    CAIRO_OPERATOR_LIGHTEN,
    CAIRO_OPERATOR_COLOR_DODGE,
    CAIRO_OPERATOR_COLOR_BURN,
    CAIRO_OPERATOR_HARD_LIGHT,
    CAIRO_OPERATOR_SOFT_LIGHT,
    CAIRO_OPERATOR_DIFFERENCE,
    CAIRO_OPERATOR_EXCLUSION,
    CAIRO_OPERATOR_CLEAR,
    CAIRO_OPERATOR_SOURCE,
    CAIRO_OPERATOR_IN,
    CAIRO_OPERATOR_OUT,
    CAIRO_OPERATOR_ATOP,
    CAIRO_OPERATOR_DEST,
    CAIRO_OPERATOR_DEST_OVER,
    CAIRO_OPERATOR_DEST_IN,
    CAIRO_OPERATOR_DEST_OUT,
    CAIRO_OPERATOR_DEST_ATOP,
    CAIRO_OPERATOR_HSL_HUE,
    CAIRO_OPERATOR_HSL_SATURATION,
    CAIRO_OPERATOR_HSL_COLOR,
    CAIRO_OPERATOR_HSL_LUMINOSITY
};

static cairo_surface_t *surface;
static cairo_surface_t *surfacefb;
static cairo_surface_t *image;

static cairo_t *cr;

#if USE_LOCK
static pthread_mutex_t cr_lock;
#endif

static cairo_t *crfb;
static cairo_font_face_t *ct[NUM_FONTS];
static FT_Library value;
static FT_Error status;
static FT_Face face[NUM_FONTS];
//static double text_xy[2];

typedef struct _cairo_linuxfb_device {
    int fb_fd;
    unsigned char *fb_data;
    long fb_screensize;
    struct fb_var_screeninfo fb_vinfo;
    struct fb_fix_screeninfo fb_finfo;
} cairo_linuxfb_device_t;

static void cairo_linuxfb_surface_destroy(void *device);
static cairo_surface_t *cairo_linuxfb_surface_create();
static void init_font_faces(void);

//---------------------------------------
//--- extern function definitions

void screen_init(void) {
    surfacefb = cairo_linuxfb_surface_create();
    if (surfacefb == NULL) {
        return;
    }
    crfb = cairo_create(surfacefb);

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 128, 64);
    cr = cairo_create(surface);

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_font_options_t *font_options = cairo_font_options_create();
    cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_SUBPIXEL);
    cairo_set_font_options(cr, font_options);
    cairo_font_options_destroy(font_options);

    init_font_faces();

    // default font
    cairo_set_font_face(cr, ct[0]);
    cairo_set_font_size(cr, 8.0);

    // config buffer
    cairo_set_operator(crfb, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(crfb, surface, 0, 0);
}

void screen_deinit(void) {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_destroy(crfb);
    cairo_surface_destroy(surfacefb);
}

//-----------------
//-- screen commands

void screen_update(void) {
    LOCK_CR
    cairo_paint(crfb);
    UNLOCK_CR
}

void screen_save(void) {
    LOCK_CR
    cairo_save(cr);
    UNLOCK_CR
}

void screen_restore(void) {
    LOCK_CR
    cairo_restore(cr);
    UNLOCK_CR
}

void screen_font_face(int i) {
    LOCK_CR
    if ((i >= 0) && (i < NUM_FONTS)) {
        cairo_set_font_face(cr, ct[i]);
    }
    UNLOCK_CR
}

void screen_font_size(double z) {
    LOCK_CR
    cairo_set_font_size(cr, z);
    UNLOCK_CR
}

void screen_aa(int s) {
    LOCK_CR
    cairo_font_options_t *font_options = cairo_font_options_create();
    if (s == 0) {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
        cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_NONE);
    } else {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
        cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_SUBPIXEL);
    }
    cairo_set_font_options(cr, font_options);
    cairo_font_options_destroy(font_options);
    UNLOCK_CR
}

void screen_level(int z) {
    z = z < 0 ? 0 : (z > 15 ? 15 : z);
    LOCK_CR
    cairo_set_source_rgb(cr, c[z], c[z], c[z]);
    UNLOCK_CR
}

void screen_line_width(double w) {
    LOCK_CR
    cairo_set_line_width(cr, w);
    UNLOCK_CR
}

void screen_line_cap(const char *style) {
    LOCK_CR
    if (strcmp(style, "round") == 0) {
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    } else if (strcmp(style, "square") == 0) {
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
    } else {
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    }
    UNLOCK_CR
}

void screen_line_join(const char *style) {
    LOCK_CR
    if (strcmp(style, "round") == 0) {
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    } else if (strcmp(style, "bevel") == 0) {
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
    } else {
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
    }
    UNLOCK_CR
}

void screen_miter_limit(double limit) {
    LOCK_CR
    cairo_set_miter_limit(cr, limit);
    UNLOCK_CR
}

void screen_move(double x, double y) {
    LOCK_CR
    cairo_move_to(cr, x, y);
    UNLOCK_CR
}

void screen_line(double x, double y) {
    LOCK_CR
    cairo_line_to(cr, x, y);
    UNLOCK_CR
}

void screen_line_rel(double x, double y) {
    LOCK_CR
    cairo_rel_line_to(cr, x, y);
    UNLOCK_CR
}

void screen_move_rel(double x, double y) {
    LOCK_CR
    cairo_rel_move_to(cr, x, y);
    UNLOCK_CR
}

void screen_curve(double x1, double y1, double x2, double y2, double x3, double y3) {
    LOCK_CR
    cairo_curve_to(cr, x1, y1, x2, y2, x3, y3);
    UNLOCK_CR
}

void screen_curve_rel(double dx1, double dy1, double dx2, double dy2, double dx3, double dy3) {
    LOCK_CR
    cairo_rel_curve_to(cr, dx1, dy1, dx2, dy2, dx3, dy3);
    UNLOCK_CR
}

void screen_arc(double x, double y, double r, double a1, double a2) {
    LOCK_CR
    cairo_arc(cr, x, y, r, a1, a2);
    UNLOCK_CR
}

void screen_rect(double x, double y, double w, double h) {
    LOCK_CR
    cairo_rectangle(cr, x, y, w, h);
    UNLOCK_CR
}

void screen_close_path(void) {
    LOCK_CR
    cairo_close_path(cr);
    UNLOCK_CR
}

void screen_stroke(void) {
    LOCK_CR
    cairo_stroke(cr);
    UNLOCK_CR
}

void screen_fill(void) {
    LOCK_CR
    cairo_fill(cr);
    UNLOCK_CR
}

void screen_text(const char *s) {
    LOCK_CR
    cairo_show_text(cr, s);
    UNLOCK_CR
}


void screen_text_right(const char *s) {    
    cairo_text_extents_t extents;    
    LOCK_CR
    cairo_text_extents(cr, s, &extents);
    cairo_rel_move_to(cr, -extents.width, 0);
    cairo_show_text(cr, s);
    UNLOCK_CR
}

void screen_text_center(const char *s) {
    cairo_text_extents_t extents;    
    LOCK_CR
    cairo_text_extents(cr, s, &extents);
    cairo_rel_move_to(cr, -extents.width * 0.5, 0);
    cairo_show_text(cr, s);
    UNLOCK_CR
}

void screen_clear(void) {
    LOCK_CR
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    UNLOCK_CR
}

void screen_text_extents(const char *s) {
    cairo_text_extents_t extents;
    LOCK_CR
    cairo_text_extents(cr, s, &extents);    
    UNLOCK_CR
    union event_data *ev = event_data_new(EVENT_SCREEN_RESULT_TEXT_EXTENTS);    
    ev->screen_result_text_extents.x_bearing = extents.x_bearing;
    ev->screen_result_text_extents.y_bearing = extents.y_bearing;
    ev->screen_result_text_extents.width = extents.width;
    ev->screen_result_text_extents.height = extents.height;
    ev->screen_result_text_extents.x_advance = extents.x_advance;
    ev->screen_result_text_extents.y_advance = extents.y_advance;
    event_post(ev);
}

void screen_peek(int x, int y, int w, int h) {
    w = (w <= (128 - x)) ? w : (128 - x);
    h = (h <= (64 - y))  ? h : (64 - y);
    char *buf = malloc(w * h);
    if (!buf) {
        return;
    }
    // NB: peek/poke do not actually access the CR,
    // but we do want to avoid torn values
    LOCK_CR
    cairo_surface_flush(surface);
    uint32_t *data = (uint32_t *)cairo_image_surface_get_data(surface);
    if (!data) {
        UNLOCK_CR
	return;
    }
    char *p = buf;
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            *p = data[j * 128 + i] & 0xF;
            p++;
        }
    }
    UNLOCK_CR
    union event_data *ev = event_data_new(EVENT_SCREEN_RESULT_PEEK);
    ev->screen_result_peek.w = w;
    ev->screen_result_peek.h = h;    
    ev->screen_result_peek.buf = buf;
    event_post(ev);
}

void screen_poke(int x, int y, int w, int h, unsigned char *buf) {
    w = (w <= (128 - x)) ? w : (128 - x);
    h = (h <= (64 - y))  ? h : (64 - y);
    // NB: peek/poke do not actually access the CR,
    // but we do want to avoid torn values
    LOCK_CR
    uint32_t *data = (uint32_t *)cairo_image_surface_get_data(surface);
    if (!data) {
        UNLOCK_CR
        return;
    }
    uint8_t *p = buf;
    uint32_t pixel;
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            pixel = *p;
            pixel = pixel | (pixel << 4);
            data[j * 128 + i] = pixel | (pixel << 8) | (pixel << 16) | (pixel << 24);
            p++;
        }
    }
    cairo_surface_mark_dirty(surface);
    UNLOCK_CR
}

void screen_rotate(double r) {
    LOCK_CR
    cairo_rotate(cr, r);
    UNLOCK_CR
}

void screen_translate(double x, double y) {
    LOCK_CR
    cairo_translate(cr, x, y);
    UNLOCK_CR
}

void screen_set_operator(int i) {
    LOCK_CR
    if (0 <= i && i <= 28) {
        cairo_set_operator(cr, ops[i]);
    }
    UNLOCK_CR
}

void screen_display_png(const char *filename, double x, double y) {
    int img_w, img_h;

    image = cairo_image_surface_create_from_png(filename);
    if (cairo_surface_status(image)) {
        fprintf(stderr, "display_png: %s\n", cairo_status_to_string(cairo_surface_status(image)));
        return;
    }

    img_w = cairo_image_surface_get_width(image);
    img_h = cairo_image_surface_get_height(image);

    LOCK_CR
    cairo_set_source_surface(cr, image, x, y);
    // cairo_paint (cr);
    cairo_rectangle(cr, x, y, img_w, img_h);
    cairo_fill(cr);
    UNLOCK_CR
    cairo_surface_destroy(image);
}

void screen_export_png(const char *s) {
    cairo_surface_write_to_png(surface, s);
}

void screen_current_point() {
    double x, y;
    LOCK_CR;
    cairo_get_current_point (cr, &x, &y);
    UNLOCK_CR;    
    union event_data *ev = event_data_new(EVENT_SCREEN_RESULT_CURRENT_POINT);
    ev->screen_result_current_point.x = x;
    ev->screen_result_current_point.y = y;
    event_post(ev);
}

//-------------------------------------------------------
//-- static function definitions

void cairo_linuxfb_surface_destroy(void *device) {
    cairo_linuxfb_device_t *dev = (cairo_linuxfb_device_t *)device;

    if (dev == NULL) {
        return;
    }

    munmap(dev->fb_data, dev->fb_screensize);
    close(dev->fb_fd);
    free(dev);
}

cairo_surface_t *cairo_linuxfb_surface_create() {
    cairo_linuxfb_device_t *device;
    cairo_surface_t *surface;

    const char *fb_name = args_framebuffer();

    device = malloc(sizeof(*device));
    if (!device) {
        fprintf(stderr, "ERROR (screen) cannot allocate memory\n");
        return NULL;
    }

    device->fb_fd = open(fb_name, O_RDWR);
    if (device->fb_fd == -1) {
        fprintf(stderr, "ERROR (screen) cannot open framebuffer device\n");
        goto handle_allocate_error;
    }

    if (ioctl(device->fb_fd, FBIOGET_VSCREENINFO, &device->fb_vinfo) == -1) {
        fprintf(stderr, "ERROR (screen) reading variable information\n");
        goto handle_ioctl_error;
    }

    device->fb_screensize = device->fb_vinfo.xres * device->fb_vinfo.yres * device->fb_vinfo.bits_per_pixel / 8;

    device->fb_data = (unsigned char *)mmap(0, device->fb_screensize, 
    PROT_READ | PROT_WRITE, MAP_SHARED, device->fb_fd, 0);

    if (device->fb_data == (unsigned char *)-1) {
        fprintf(stderr, "ERROR (screen) failed to map framebuffer device to memory\n");
        goto handle_ioctl_error;
    }

    if (ioctl(device->fb_fd, FBIOGET_FSCREENINFO, &device->fb_finfo) == -1) {
        fprintf(stderr, "ERROR (screen) reading fixed information\n");
        goto handle_ioctl_error;
    }

    surface = cairo_image_surface_create_for_data(
        device->fb_data, CAIRO_FORMAT_RGB16_565, device->fb_vinfo.xres, device->fb_vinfo.yres,
        cairo_format_stride_for_width(CAIRO_FORMAT_RGB16_565, device->fb_vinfo.xres));
    cairo_surface_set_user_data(surface, NULL, device, &cairo_linuxfb_surface_destroy);

    return surface;

handle_ioctl_error:
    close(device->fb_fd);
handle_allocate_error:
    free(device);
    return NULL;
}


 void init_font_faces(void) { 
    
    status = FT_Init_FreeType(&value);
    if (status != 0) {
        fprintf(stderr, "ERROR (screen) freetype init\n");
        return;
    }

    int font_idx = 0;
    strcpy(font_path[font_idx++], "04B_03__.TTF");
    strcpy(font_path[font_idx++], "liquid.ttf");
    strcpy(font_path[font_idx++], "Roboto-Thin.ttf");
    strcpy(font_path[font_idx++], "Roboto-Light.ttf");
    strcpy(font_path[font_idx++], "Roboto-Regular.ttf");
    strcpy(font_path[font_idx++], "Roboto-Medium.ttf");
    strcpy(font_path[font_idx++], "Roboto-Bold.ttf");
    strcpy(font_path[font_idx++], "Roboto-Black.ttf");
    strcpy(font_path[font_idx++], "Roboto-ThinItalic.ttf");
    strcpy(font_path[font_idx++], "Roboto-LightItalic.ttf");
    strcpy(font_path[font_idx++], "Roboto-Italic.ttf");
    strcpy(font_path[font_idx++], "Roboto-MediumItalic.ttf");
    strcpy(font_path[font_idx++], "Roboto-BoldItalic.ttf");
    strcpy(font_path[font_idx++], "Roboto-BlackItalic.ttf");
    strcpy(font_path[font_idx++], "VeraBd.ttf");
    strcpy(font_path[font_idx++], "VeraBI.ttf");
    strcpy(font_path[font_idx++], "VeraIt.ttf");
    strcpy(font_path[font_idx++], "VeraMoBd.ttf");
    strcpy(font_path[font_idx++], "VeraMoBI.ttf");
    strcpy(font_path[font_idx++], "VeraMoIt.ttf");
    strcpy(font_path[font_idx++], "VeraMono.ttf");
    strcpy(font_path[font_idx++], "VeraSeBd.ttf");
    strcpy(font_path[font_idx++], "VeraSe.ttf");
    strcpy(font_path[font_idx++], "Vera.ttf");
    //------------------
    // bitmap fonts
    strcpy(font_path[font_idx++], "bmp/tom-thumb.bdf");
    strcpy(font_path[font_idx++], "bmp/creep.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-10b.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-10r.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-13b.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-13b-i.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-13r.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-13r-i.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-16b.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-16b-i.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-16r.bdf");
    strcpy(font_path[font_idx++], "bmp/ctrld-fixed-16r-i.bdf");
    strcpy(font_path[font_idx++], "bmp/scientifica-11.bdf");
    strcpy(font_path[font_idx++], "bmp/scientificaBold-11.bdf");
    strcpy(font_path[font_idx++], "bmp/scientificaItalic-11.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u12b.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u12n.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u14b.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u14n.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u14v.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u16b.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u16n.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u16v.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u18b.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u18n.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u20b.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u20n.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u22b.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u22n.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u24b.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u24n.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u28b.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u28n.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u32b.bdf");
    strcpy(font_path[font_idx++], "bmp/ter-u32n.bdf");
    strcpy(font_path[font_idx++], "bmp/unscii-16-full.pcf");
    strcpy(font_path[font_idx++], "bmp/unscii-16.pcf");
    strcpy(font_path[font_idx++], "bmp/unscii-8-alt.pcf");
    strcpy(font_path[font_idx++], "bmp/unscii-8-fantasy.pcf");
    strcpy(font_path[font_idx++], "bmp/unscii-8-mcr.pcf");
    strcpy(font_path[font_idx++], "bmp/unscii-8.pcf");
    strcpy(font_path[font_idx++], "bmp/unscii-8-tall.pcf");
    strcpy(font_path[font_idx++], "bmp/unscii-8-thin.pcf");

    assert(font_idx == NUM_FONTS);

    fprintf(stderr, "fonts: \n");
    for (int i = 0; i < NUM_FONTS; ++i) {
        fprintf(stderr, "  %d: %s\n", i, font_path[i]);
    }

    int path_bytes = 512;
    char *path = malloc(path_bytes);
    for (int i = 0; i < NUM_FONTS; i++) {
        status = snprintf(path, path_bytes, "%s/norns/resources/%s", getenv("HOME"), font_path[i]);
        if (status > path_bytes) {
            fprintf(stderr, "ERROR (screen) font path too long: %s\n", path);
        }

        status = FT_New_Face(value, path, 0, &face[i]);
        if (status != 0) {
            fprintf(stderr, "ERROR (screen) couldn't load font: %s\n", path);
        } else {
            ct[i] = cairo_ft_font_face_create_for_ft_face(face[i], 0);
        }
    }
    free(path);

}
