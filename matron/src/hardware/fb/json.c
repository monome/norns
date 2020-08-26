#include <cairo.h>
#include <stdlib.h>

#include "hardware/fb/matron_fb.h"
#include "hardware/fb/base64/base64.h"

typedef struct _cairo_json_fb_device {
    void (*write)(const char *data, size_t len);
    void (*flush)(void);
} cairo_json_fb_device_t;

static void json_fb_destroy(matron_fb_t *f);
static void json_fb_paint(matron_fb_t *f);
static void json_fb_bind(matron_fb_t *f, cairo_surface_t *surface);

static void cairo_json_fb_surface_destroy(void *device);
static cairo_surface_t *cairo_json_fb_surface_create(cairo_json_fb_device_t *device);
static void cairo_json_fb_surface_destroy(void *device);
static cairo_status_t cairo_json_write(void* closure, const unsigned char *data, unsigned int length);

int json_fb_init(matron_fb_t *fb,
                 void (*write)(const char *data, size_t len), void (*flush)(void)) {
    cairo_json_fb_device_t *device;
    fb->data = malloc(sizeof(cairo_json_fb_device_t));
    device = (cairo_json_fb_device_t*)fb->data;
    if (!fb->data) {
        fprintf(stderr, "ERROR (screen - json_fb) cannot allocate memory\n");
	return -1;
    }
    fb->surface = cairo_json_fb_surface_create(device);
    if (!fb->surface) {
        fprintf(stderr, "ERROR (screen - json_fb) cannot create surface\n");
        free(fb->data);
        return -1;
    }
    fb->cairo = cairo_create(fb->surface);
    if (!fb->cairo) {
        fprintf(stderr, "ERROR (screen - json_fb) cannot create cairo context\n");
        cairo_surface_destroy(fb->surface);
	free(fb->data);
	return -1;
    }

    fb->destroy = &json_fb_destroy;
    fb->paint = &json_fb_paint;
    fb->bind = &json_fb_bind;
    device->write = write;
    device->flush = flush;
    return 0;
}

static void json_fb_destroy(matron_fb_t *fb) {
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
    
    device->write(json_frame_header, sizeof(json_frame_header));
    cairo_surface_write_to_png_stream(fb->surface, cairo_json_write, fb);
    device->write(json_frame_footer, sizeof(json_frame_footer));
    device->flush();
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
    unsigned int b64_size = b64e_size(length);
    unsigned char *output = calloc(1, b64_size);
    if (output == NULL) {
        fprintf(stderr, "ERROR (screen - json_fb): failed to allocate %d for b64 encoding\n", b64_size);
        return CAIRO_STATUS_WRITE_ERROR;
    }
    b64_encode(data, length, output);
    device->write((char*)output, b64_size);
    free(output);
    return CAIRO_STATUS_SUCCESS;
}
