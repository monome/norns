#include <cairo.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/fb/matron_fb.h"
#include "hardware/fb/base64/base64.h"

#define PNG_BUFFER_SIZE 1024

struct _cairo_json_fb_device;

typedef struct _cairo_json_fb_device {
    unsigned char *buf;
    size_t buf_pos;

    int (*write)(struct _cairo_json_fb_device *device, const char *data, size_t len);
    void (*flush)(struct _cairo_json_fb_device *device);
} cairo_json_fb_device_t;

static cairo_surface_t* json_fb_init(matron_fb_t *fb);
static void json_fb_destroy(matron_fb_t *fb);
static void json_fb_paint(matron_fb_t *fb);
static void json_fb_bind(matron_fb_t *fb, cairo_surface_t *surface);

static void cairo_json_fb_surface_destroy(void *device);
static cairo_surface_t *cairo_json_fb_surface_create(cairo_json_fb_device_t *device);
static void cairo_json_fb_surface_destroy(void *device);
static cairo_status_t cairo_json_write(void* closure, const unsigned char *data, unsigned int length);

static int stream_fb_write(cairo_json_fb_device_t *device, const char *data, size_t length);
static void stream_fb_flush(cairo_json_fb_device_t *device);

fb_ops_t json_fb_ops = {
    .name = "web",
    .data_size = sizeof(cairo_json_fb_device_t),
    .init = json_fb_init,
    .destroy = json_fb_destroy,
    .paint = json_fb_paint,
    .bind = json_fb_bind,
};

cairo_surface_t* json_fb_init(matron_fb_t *fb) {
    cairo_json_fb_device_t *device = fb->data;
    device->buf = malloc(PNG_BUFFER_SIZE);
    if (!device->buf) {
        return NULL;
    }
    device->write = stream_fb_write;
    device->flush = stream_fb_flush;
    return cairo_json_fb_surface_create(device);
}

static void json_fb_destroy(matron_fb_t *fb) {
    cairo_json_fb_device_t *device = fb->data;
    free(device->buf);
    cairo_destroy(fb->cairo);
    cairo_surface_destroy(fb->surface);
    free(fb->data);
}

static const char json_frame_header[] =
    "{\"type\":\"fb\",\"data\":\"data:image/png;base64,";
static const char json_frame_footer[] =
    "\"}";

static void json_fb_paint(matron_fb_t *fb) {  
    cairo_json_fb_device_t *device = (cairo_json_fb_device_t *)fb->data;
    cairo_paint(fb->cairo);
    device->buf_pos = 0;
    cairo_surface_write_to_png_stream(fb->surface, cairo_json_write, fb);
    device->flush(device);
}

static void json_fb_bind(matron_fb_t *fb, cairo_surface_t *surface) {
    cairo_set_operator(fb->cairo, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(fb->cairo, surface, 0, 0);
}

static cairo_surface_t *cairo_json_fb_surface_create(cairo_json_fb_device_t *device) {
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB16_565, 128, 64);
    if (!surface) {
        fprintf(stderr, "screen: could not create json framebuffer surface\n");
        return NULL;
    }
    cairo_surface_set_user_data(surface, NULL, device, &cairo_json_fb_surface_destroy);
    fprintf(stderr, "screen: created json framebuffer surface\n");
    return surface;
}

static void cairo_json_fb_surface_destroy(void *device) {
    cairo_json_fb_device_t *dev = (cairo_json_fb_device_t *)device;
    if (dev == NULL) {
        return;
    }
    free(dev);
}

static cairo_status_t cairo_json_write(void* closure, const unsigned char *data, unsigned int length)
{
    matron_fb_t *fb = (matron_fb_t *)closure;
    cairo_json_fb_device_t *device = (cairo_json_fb_device_t *)fb->data;
    if (device->write(device, (char*)data, length)) {
        return CAIRO_STATUS_WRITE_ERROR;
    }
    return CAIRO_STATUS_SUCCESS;
}

static int stream_fb_write(cairo_json_fb_device_t *device, const char *data, size_t length)
{
    if (device->buf_pos + length > PNG_BUFFER_SIZE) {
        return -1;
    }
    memcpy(device->buf + device->buf_pos, data, length);
    device->buf_pos += length;
    return 0;
}

static void stream_fb_flush(cairo_json_fb_device_t *device)
{
    unsigned int b64_size = b64e_size(device->buf_pos);
    unsigned char *b64_buf = calloc(b64_size + 1, sizeof(char));
    b64_encode(device->buf, device->buf_pos, b64_buf);
    fwrite(json_frame_header, 1, sizeof(json_frame_header) - 1, stderr);
    fwrite(b64_buf, 1, b64_size, stderr);
    fwrite(json_frame_footer, 1, sizeof(json_frame_footer) - 1, stderr);
    fwrite("\n", 1, 1, stderr);
    fflush(stderr);
}
