#include <cairo.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/io.h"
#include "hardware/screen/base64/base64.h"

#define PNG_BUFFER_SIZE 1024

struct _screen_json_priv;

typedef struct _screen_json_priv {
    unsigned char *buf;
    size_t buf_pos;

    int (*write)(struct _screen_json_priv *priv, const char *data, size_t len);
    void (*flush)(struct _screen_json_priv *priv);
} screen_json_priv_t;

static int screen_json_config(matron_io_t *io, lua_State *l);
static int screen_json_setup(matron_io_t *io);
static void screen_json_destroy(matron_io_t *io);
static void screen_json_paint(matron_fb_t *fb);
static void screen_json_bind(matron_fb_t *fb, cairo_surface_t *surface);

static void screen_json_surface_destroy(void *device);
static cairo_surface_t *screen_json_surface_create(screen_json_priv_t *device);
static cairo_status_t cairo_json_write(void* closure, const unsigned char *data, unsigned int length);

static int screen_json_write(screen_json_priv_t *device, const char *data, size_t length);
static void screen_json_flush(screen_json_priv_t *device);

screen_ops_t screen_json_ops = {
    .io_ops.name      = "screen:json",
    .io_ops.type      = IO_SCREEN,
    .io_ops.data_size = sizeof(screen_json_priv_t),
    .io_ops.config    = screen_json_config,
    .io_ops.setup     = screen_json_setup,
    .io_ops.destroy   = screen_json_destroy,

    .paint            = screen_json_paint,
    .bind             = screen_json_bind,
};

int screen_json_config(matron_io_t *io, lua_State *l) {
    (void)io;
    (void)l;
    return 0;
}

int screen_json_setup(matron_io_t *io) {
    matron_fb_t *fb = (matron_fb_t *)io;
    screen_json_priv_t *priv = io->data;
    priv->buf = malloc(PNG_BUFFER_SIZE);
    if (!priv->buf) {
        fprintf(stderr, "ERROR (%s) out of memory\n", io->ops->name);
        return -1;
    }
    priv->write = screen_json_write;
    priv->flush = screen_json_flush;
    fb->surface = screen_json_surface_create(priv);
    if (!fb->surface) {
        fprintf(stderr, "ERROR (%s) failed to create surface\n", io->ops->name);
        return -1;
    }
    fb->cairo = cairo_create(fb->surface);
    return 0;
}

static void screen_json_destroy(matron_io_t *io) {
    matron_fb_t *fb = (matron_fb_t *)io;
    screen_json_priv_t *priv = io->data;
    free(priv->buf);
    cairo_destroy(fb->cairo);
    cairo_surface_destroy(fb->surface);
}

static const char json_frame_header[] =
    "{\"type\":\"fb\",\"data\":\"data:image/png;base64,";
static const char json_frame_footer[] =
    "\"}";

static void screen_json_paint(matron_fb_t *fb) {
    screen_json_priv_t *priv = fb->io.data;
    cairo_paint(fb->cairo);
    priv->buf_pos = 0;
    cairo_surface_write_to_png_stream(fb->surface, cairo_json_write, fb);
    priv->flush(priv);
}

static void screen_json_bind(matron_fb_t *fb, cairo_surface_t *surface) {
    cairo_set_operator(fb->cairo, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(fb->cairo, surface, 0, 0);
}

static cairo_surface_t *screen_json_surface_create(screen_json_priv_t *priv) {
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB16_565, 128, 64);
    if (!surface) {
        fprintf(stderr, "screen: could not create json framebuffer surface\n");
        return NULL;
    }
    cairo_surface_set_user_data(surface, NULL, priv, &screen_json_surface_destroy);
    fprintf(stderr, "screen: created json framebuffer surface\n");
    return surface;
}

static void screen_json_surface_destroy(void *data) {
    screen_json_priv_t *priv = data;
    if (priv == NULL) {
        return;
    }
    free(priv);
}

static cairo_status_t cairo_json_write(void* closure, const unsigned char *data, unsigned int length)
{
    matron_fb_t *fb = (matron_fb_t *)closure;
    screen_json_priv_t *priv = fb->io.data;
    if (priv->write(priv, (char*)data, length)) {
        return CAIRO_STATUS_WRITE_ERROR;
    }
    return CAIRO_STATUS_SUCCESS;
}

static int screen_json_write(screen_json_priv_t *priv, const char *data, size_t length)
{
    if (priv->buf_pos + length > PNG_BUFFER_SIZE) {
        return -1;
    }
    memcpy(priv->buf + priv->buf_pos, data, length);
    priv->buf_pos += length;
    return 0;
}

static void screen_json_flush(screen_json_priv_t *priv)
{
    unsigned int b64_size = b64e_size(priv->buf_pos);
    unsigned char *b64_buf = calloc(b64_size + 1, sizeof(char));
    b64_encode(priv->buf, priv->buf_pos, b64_buf);
    fwrite(json_frame_header, 1, sizeof(json_frame_header) - 1, stderr);
    fwrite(b64_buf, 1, b64_size, stderr);
    fwrite(json_frame_footer, 1, sizeof(json_frame_footer) - 1, stderr);
    fwrite("\n", 1, 1, stderr);
    fflush(stderr);
}
