/*
 * screen.c
 *
 * cairo drawing (and a little ioctl)
 */

#include <assert.h>
#include <cairo.h>
#include <cairo-ft.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "events.h"
#include "event_types.h"
#include "screen.h"
#include "screen_results.h"
#include "hardware/io.h"
#include "hardware/screen.h"
#include "hardware/screen/ssd1322.h"

#define NUM_FONTS 68
#define NUM_OPS 29

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
static cairo_surface_t *image;
static cairo_t *cr;
static cairo_t *cr_primary;
static bool surface_may_have_color = false;


static cairo_font_face_t *ct[NUM_FONTS];
static FT_Library value;
static FT_Error status;
static FT_Face face[NUM_FONTS];

static void init_font_faces(void);

//---------------------------------------
//--- extern function definitions

void screen_init(void) {
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 128, 64);
    cr = cr_primary = cairo_create(surface);

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
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, surface, 0, 0);
}

//-----------------
//-- screen commands

void screen_text_right(const char *s) {
    cairo_text_extents_t extents;    
    cairo_text_extents(cr, s, &extents);
    cairo_rel_move_to(cr, -extents.width, 0);
    cairo_show_text(cr, s);
}

void screen_text_center(const char *s) {
    cairo_text_extents_t extents;    
    cairo_text_extents(cr, s, &extents);
    cairo_rel_move_to(cr, -extents.width * 0.5, 0);
    cairo_show_text(cr, s);
}

void screen_text_trim(char *s, double w) {
    cairo_text_extents_t extents;
    int n = strlen(s);
    do {
	s[n--] = '\0';
	cairo_text_extents(cr, s, &extents);
	if (n <= 0) {
	    return;
	}
    } while (extents.width > w);
    cairo_show_text(cr, s);    
}


void screen_current_point() {
    double x, y;
    cairo_get_current_point (cr, &x, &y);
    union screen_results_data *results = screen_results_data_new(SCREEN_RESULTS_CURRENT_POINT);
    results->current_point.x = x;
    results->current_point.y = y;
    screen_results_post(results);
}

//-------------------------------------------------------
//-- static function definitions

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
    //------------------
    // not specifically bitmap but added to the tail of the font list to avoid breaking changes
    strcpy(font_path[font_idx++], "Particle.ttf");

    assert(font_idx == NUM_FONTS);

#if 0 // is this really necessary?
    fprintf(stderr, "fonts: \n");
    for (int i = 0; i < NUM_FONTS; ++i) {
        fprintf(stderr, "  %d: %s\n", i, font_path[i]);
    }
#endif

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

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_font_options_t *font_options = cairo_font_options_create();
    cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_GRAY);
    cairo_set_font_options(cr, font_options);
    cairo_font_options_destroy(font_options);

    // default font
    cairo_set_font_face(cr, ct[0]);
    cairo_set_font_size(cr, 8.0);

    fprintf(stderr, "font setup OK.\n");
#ifdef NORNS_DESKTOP
    matron_io_t *io;
    TAILQ_FOREACH(io, &io_queue, entries) {
        if (io->ops->type != IO_SCREEN) continue;
        matron_fb_t *fb = (matron_fb_t *)io;
        screen_ops_t *fb_ops = (screen_ops_t *)io->ops;
        fb_ops->bind(fb, surface);
    }
#endif
}

void screen_deinit(void) {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void screen_update(void) {

#ifdef NORNS_DESKTOP
    matron_io_t *io;
    TAILQ_FOREACH(io, &io_queue, entries) {
        if (io->ops->type != IO_SCREEN) continue;
        matron_fb_t *fb = (matron_fb_t *)io;
        screen_ops_t *fb_ops = (screen_ops_t *)io->ops;
        fb_ops->paint(fb);
    }
    return;
#endif

    cairo_surface_flush(surface);
    ssd1322_update(surface, surface_may_have_color);
}

void screen_save(void) {
    cairo_save(cr);
}

void screen_restore(void) {
    cairo_restore(cr);
}

void screen_font_face(int i) {
    if ((i >= 0) && (i < NUM_FONTS)) {
        cairo_set_font_face(cr, ct[i]);
    }
}

void screen_font_size(double z) {
    cairo_set_font_size(cr, z);
}

void screen_aa(int s) {
    cairo_font_options_t *font_options = cairo_font_options_create();
    if (s == 0) {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
        cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_NONE);
    } else {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
        cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_GRAY);
    }
    cairo_set_font_options(cr, font_options);
    cairo_font_options_destroy(font_options);
}

void screen_brightness(int v) {
    if (v < 0) {
        v=0;
    }
    if (v > 15) {
        v=15;
    }

    // True range of pre-charge voltage, AKA "brightness" is 0-31.
    // Below 16 is too dark for the lowest screen levels, so the range
    // is limited and offset.
    v += 16;

    ssd1322_set_brightness((uint8_t) v);
}

void screen_contrast(int c){
    if (c < 0) {
        c=0;
    }
    if (c > 255) {
        c=255;
    }
    ssd1322_set_contrast((uint8_t) c);
}

void screen_gamma(double g) {
    if (g < 0.0) {
        g=0;
    }

    ssd1322_set_gamma(g);
}

void screen_invert(int inverted){
    ssd1322_set_display_mode((inverted != 0) ? SSD1322_DISPLAY_MODE_INVERT : SSD1322_DISPLAY_MODE_NORMAL);
}

void screen_level(int z) {
    z = z < 0 ? 0 : (z > 15 ? 15 : z);
    cairo_set_source_rgb(cr, c[z], c[z], c[z]);
}

void screen_line_width(double w) {
    cairo_set_line_width(cr, w);
}

void screen_line_cap(const char *style) {
    if (strcmp(style, "round") == 0) {
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    } else if (strcmp(style, "square") == 0) {
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
    } else {
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    }
}

void screen_line_join(const char *style) {
    if (strcmp(style, "round") == 0) {
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    } else if (strcmp(style, "bevel") == 0) {
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
    } else {
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
    }
}

void screen_miter_limit(double limit) {
    cairo_set_miter_limit(cr, limit);
}

void screen_move(double x, double y) {
    cairo_move_to(cr, x, y);
}

void screen_line(double x, double y) {
    cairo_line_to(cr, x, y);
}

void screen_line_rel(double x, double y) {
    cairo_rel_line_to(cr, x, y);
}

void screen_move_rel(double x, double y) {
    cairo_rel_move_to(cr, x, y);
}

void screen_curve(double x1, double y1, double x2, double y2, double x3, double y3) {
    cairo_curve_to(cr, x1, y1, x2, y2, x3, y3);
}

void screen_curve_rel(double dx1, double dy1, double dx2, double dy2, double dx3, double dy3) {
    cairo_rel_curve_to(cr, dx1, dy1, dx2, dy2, dx3, dy3);
}

void screen_arc(double x, double y, double r, double a1, double a2) {
    cairo_arc(cr, x, y, r, a1, a2);
}

void screen_rect(double x, double y, double w, double h) {
    cairo_rectangle(cr, x, y, w, h);
}

void screen_close_path(void) {
    cairo_close_path(cr);
}

void screen_stroke(void) {
    cairo_stroke(cr);
}

void screen_fill(void) {
    cairo_fill(cr);
}

void screen_text(const char *s) {
    cairo_show_text(cr, s);
}

void screen_clear(void) {
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    surface_may_have_color = false;
}

void screen_text_extents(const char *s) {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, s, &extents);   
    union screen_results_data* results = screen_results_data_new(SCREEN_RESULTS_TEXT_EXTENTS);
    results->text_extents.x_bearing = extents.x_bearing;
    results->text_extents.y_bearing = extents.y_bearing;
    results->text_extents.width = extents.width;
    results->text_extents.height = extents.height;
    results->text_extents.x_advance = extents.x_advance;
    results->text_extents.y_advance = extents.y_advance;
    screen_results_post(results);
}

extern void screen_export_png(const char *s) {
	cairo_surface_write_to_png(surface, s);
}

extern void screen_export_screenshot(const char *s) {
    static cairo_surface_t *png;
    static cairo_t *temp; // for bg fill
    // width = 640 (128*4 pixels with 64 pixel black border)
    // hieght = 384 (64*4 pixels plus border) 
    png = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 640, 384);
    temp = cairo_create(png);
    // fill
    cairo_set_source_rgb(temp, 0, 0, 0);
    cairo_rectangle(temp, 0, 0, 640, 384);
    cairo_fill(temp);
    // copy big pixles
    uint32_t *src = (uint32_t *)cairo_image_surface_get_data(surface);
    uint32_t *dst = (uint32_t *)cairo_image_surface_get_data(png);
    if (!src || !dst) return;
    dst += 64 + 640*64;
    for(int y=0;y<64;y++) {
        for(int x=0;x<128;x++) {
            // FIXME: needs some sort of gamma correction?
            uint32_t p = *src++ | 0xFF000000; // FF for alpha
            for(int xx=0;xx<4;xx++) {
                *(dst+1920) = p;
                *(dst+1280) = p;
                *(dst+640) = p;
                *dst++ = p;
            }
        }
        dst += 640*4 - 128*4;
    }
    // cleanup
    cairo_destroy(temp);
    cairo_surface_write_to_png(png, s);
    cairo_surface_destroy((cairo_surface_t *)png);
}

void screen_display_png(const char *filename, double x, double y) {
    int img_w, img_h;

    image = cairo_image_surface_create_from_png(filename);

    if (cairo_surface_status(image)) {
        fprintf(stderr, "display_png: %s\n", cairo_status_to_string(cairo_surface_status(image)));
        return;
    }

    surface_may_have_color = true;

    img_w = cairo_image_surface_get_width(image);
    img_h = cairo_image_surface_get_height(image);

    cairo_save(cr);
    cairo_set_source_surface(cr, image, x, y);
    cairo_rectangle(cr, x, y, img_w, img_h);
    cairo_fill(cr);
    cairo_surface_destroy(image);
    cairo_restore(cr);
}

void screen_peek(int x, int y, int w, int h) {
    w = (w <= (128 - x)) ? w : (128 - x);
    h = (h <= (64 - y))  ? h : (64 - y);
    char *buf = malloc(w * h);
    if (!buf) {
        return;
    }
    cairo_surface_flush(surface);
    uint32_t *data = (uint32_t *)cairo_image_surface_get_data(surface);
    if (!data) {
        fprintf(stderr, "ERROR: screen_peek: no data\n");
        free(buf);
	    return;
    }
    char *p = buf;
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            *p = data[j * 128 + i] & 0xF;
            p++;
        }
    }
    union screen_results_data *results = screen_results_data_new(SCREEN_RESULTS_PEEK);
    results->peek.w = w;
    results->peek.h = h;    
    results->peek.buf = buf;
    screen_results_post(results);
}

void screen_poke(int x, int y, int w, int h, unsigned char *buf) {
    w = (w <= (128 - x)) ? w : (128 - x);
    h = (h <= (64 - y))  ? h : (64 - y);
    // NB: peek/poke do not actually access the CR,
    // but we do want to avoid torn values
    uint32_t *data = (uint32_t *)cairo_image_surface_get_data(surface);
    if (!data) {
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
}

void screen_rotate(double r) {
    cairo_rotate(cr, r);
}

void screen_translate(double x, double y) {
    cairo_translate(cr, x, y);
}

void screen_set_operator(int i) {
    if (0 <= i && i <= 28) {
        cairo_set_operator(cr, ops[i]);
    }
}

screen_surface_t *screen_surface_new(double width, double height) {
    int w = (int)floor(width);
    int h = (int)floor(height);
    cairo_format_t format = CAIRO_FORMAT_ARGB32;
    cairo_surface_t *image = cairo_image_surface_create(format, w, h);
    cairo_status_t status = cairo_surface_status(image);
    if (status == CAIRO_STATUS_SUCCESS) {
        return (screen_surface_t *)image;
    }
    fprintf(stderr, "surface_new: %s (%d)\n", cairo_status_to_string(status), status);
    return NULL;
}

screen_surface_t *screen_surface_load_png(const char *filename) {
    cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
    if (cairo_surface_status(image)) {
        fprintf(stderr, "load_png: %s\n", cairo_status_to_string(cairo_surface_status(image)));
        return NULL;
    }
    return (screen_surface_t *)image;
}

void screen_surface_free(screen_surface_t *s) {
    cairo_surface_destroy((cairo_surface_t *)s);
}

bool screen_surface_get_extents(screen_surface_t *s, screen_surface_extents_t *e) {
    cairo_surface_t *image = (cairo_surface_t *)s;
    e->width = cairo_image_surface_get_width(image);
    e->height = cairo_image_surface_get_height(image);
    return true;
}

void screen_surface_display(screen_surface_t *s, double x, double y) {
    cairo_surface_t *image = (cairo_surface_t *)s;
    int width = cairo_image_surface_get_width(image);
    int height = cairo_image_surface_get_height(image);

    cairo_save(cr);
    cairo_set_source_surface(cr, image, x, y);
    cairo_rectangle(cr, x, y, width, height);
    cairo_fill(cr);
    cairo_restore(cr);
}

void screen_surface_display_region(screen_surface_t *s,
                                   double left, double top, double width, double height,
                                   double x, double y) {
    cairo_surface_t *image = (cairo_surface_t *)s;
    cairo_save(cr);
    cairo_set_source_surface(cr, image, -left + x, -top + y);
    cairo_rectangle(cr, x, y, width, height);
    cairo_fill(cr);
    cairo_restore(cr);
}

screen_context_t *screen_context_new(screen_surface_t *target) {
    cairo_surface_t *image = (cairo_surface_t *)target;
    // NOTE: cairo_create increases the ref count on image which avoids a double
    // free and allows the image and context to be freed in any order
    cairo_t *context = cairo_create(image);
    cairo_status_t status = cairo_status(context);
    if (status == CAIRO_STATUS_SUCCESS) {
        return (screen_context_t *)context;
    }
    fprintf(stderr, "context_new: %s (%d)\n", cairo_status_to_string(status), status);
    return NULL;

    return (screen_context_t *)context;
}

void screen_context_free(screen_context_t *context) {
    cairo_destroy((cairo_t *)context);
}

const screen_context_t *screen_context_get_current(void) {
    return (const screen_context_t *)cr;
}

inline static void _screen_context_set(cairo_t *cr_incoming) {
    // return early if attempting to assign the same context
    if (cr_incoming == cr) {
        return;
    }

    // manage ref count for contexts which aren't the primary context
    if (cr_incoming != cr_primary) {
        cairo_reference(cr_incoming);
    }

    // decrement ref count on current context if it is not the primary
    if (cr != cr_primary) {
        cairo_destroy(cr);
    }

    cr = cr_incoming;
}

void screen_context_set(const screen_context_t *context) {
    _screen_context_set((cairo_t *)context);
}

void screen_context_set_primary(void) {
    _screen_context_set(cr_primary);
}
