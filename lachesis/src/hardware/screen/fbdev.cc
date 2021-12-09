#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cairo.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "args.h"
#include "hardware/io.h"
#include "hardware/screen/screens.h"

typedef struct _screen_fbdev_priv {
    char *dev;
    int fb_fd;
    unsigned char *fb_data;
    long fb_screensize;
    struct fb_var_screeninfo fb_vinfo;
    struct fb_fix_screeninfo fb_finfo;
} screen_fbdev_priv_t;

static int screen_fbdev_config(matron_io_t *io, lua_State *l);
static int screen_fbdev_setup(matron_io_t *io);
static void screen_fbdev_destroy(matron_io_t *io);
static void screen_fbdev_paint(matron_fb_t *fb);
static void screen_fbdev_bind(matron_fb_t *fb, cairo_surface_t *surface);

static void screen_fbdev_surface_destroy(void *device);
static cairo_surface_t *screen_fbdev_surface_create(screen_fbdev_priv_t *priv, const char *fb_name);

/*
screen_ops_t screen_fbdev_ops = {
    .io_ops.name      = "screen:fbdev",
    .io_ops.type      = IO_SCREEN,
    .io_ops.data_size = sizeof(screen_fbdev_priv_t),    
    .io_ops.config    = screen_fbdev_config,
    .io_ops.setup     = screen_fbdev_setup,
    .io_ops.destroy   = screen_fbdev_destroy,

    .paint = screen_fbdev_paint,
    .bind  = screen_fbdev_bind,
};
*/

screen_ops_t screen_fbdev_ops = {
    {                  // .io_ops
        "screen:fbdev",               // .name
         IO_SCREEN,                  // .type
         sizeof(screen_fbdev_priv_t), // .data_size
         screen_fbdev_config,         // .config
         screen_fbdev_setup,          // .setup
         screen_fbdev_destroy,        // .destroy
    },
    screen_fbdev_paint,    // .paint
    screen_fbdev_bind,     // .bind
};



int screen_fbdev_config(matron_io_t *io, lua_State *l) {
    screen_fbdev_priv_t *priv = (screen_fbdev_priv_t *)io->data;
    lua_pushstring(l, "dev");
    lua_gettable(l, -2);
    if (lua_isstring(l, -1)) {
        const char *dev = lua_tostring(l, -1);
        if (!(priv->dev = (char*)malloc(strlen(dev) + 1))) {
            fprintf(stderr, "ERROR (screen:fbdev) no memory\n");
            lua_settop(l, 0);
            return -1;
        }
        strcpy(priv->dev, dev);
    } else if (lua_isnil(l, -1)) {
        if (!(priv->dev = (char*)malloc(9))) {
            fprintf(stderr, "ERROR (screen:fbdev) no memory\n");
            free(priv->dev);
            lua_settop(l, 0);
            return -1;
        }
        strcpy(priv->dev, "/dev/fb0");
        fprintf(stderr, "screen:fbdev: no 'dev', using %s\n", priv->dev); 
    } else {
        fprintf(stderr, "ERROR (screen:fbdev) config option 'dev' should be a string\n");
        lua_settop(l, 0);
        return -1;
    }
    lua_pop(l, 1);
    return 0;
}

int screen_fbdev_setup(matron_io_t *io) {
    matron_fb_t *fb = (matron_fb_t *)io;
    screen_fbdev_priv_t *priv = (screen_fbdev_priv_t *)io->data;
    fb->surface = screen_fbdev_surface_create(priv, priv->dev);
    if (!fb->surface) {
        fprintf(stderr, "ERROR (screen:fbdev) failed to create surface '%s'\n", priv->dev);
        free(priv->dev);
        return -1;
    }
    fb->cairo = cairo_create(fb->surface);
    return 0;
}

static void screen_fbdev_destroy(matron_io_t *io) {
    matron_fb_t *fb = (matron_fb_t *)io; 
    screen_fbdev_priv_t *priv = (screen_fbdev_priv_t *)io->data;
    cairo_destroy(fb->cairo);
    cairo_surface_destroy(fb->surface);
    free(priv->dev);
}

static void screen_fbdev_paint(matron_fb_t *fb) {
    cairo_paint(fb->cairo);
}

static void screen_fbdev_bind(matron_fb_t *fb, cairo_surface_t *surface) {
    cairo_set_operator(fb->cairo, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(fb->cairo, surface, 0, 0);
}

static void screen_fbdev_surface_destroy(void *data) {
    screen_fbdev_priv_t *priv =  (screen_fbdev_priv_t *)data;

    if (priv == NULL) {
        return;
    }

    munmap(priv->fb_data, priv->fb_screensize);
    close(priv->fb_fd);
    free(priv);
}

static cairo_surface_t *screen_fbdev_surface_create(screen_fbdev_priv_t *priv, const char *dev) {
    cairo_surface_t *surface;

    // Open the file for reading and writing
    priv->fb_fd = open(dev, O_RDWR);
    if (priv->fb_fd <= 0) {
        fprintf(stderr, "ERROR (screen) cannot open framebuffer device\n");
        goto handle_allocate_error;
    }

    // Get variable screen information
    if (ioctl(priv->fb_fd, FBIOGET_VSCREENINFO, &priv->fb_vinfo) == -1) {
        fprintf(stderr, "ERROR (screen) reading variable information\n");
        goto handle_ioctl_error;
    }

    // Figure out the size of the screen in bytes
    priv->fb_screensize = priv->fb_vinfo.xres * priv->fb_vinfo.yres * priv->fb_vinfo.bits_per_pixel / 8;

    // Map the device to memory
    priv->fb_data =
        (unsigned char *)mmap(0, priv->fb_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, priv->fb_fd, 0);

    if (priv->fb_data == (unsigned char *)-1) {
        fprintf(stderr, "ERROR (screen) failed to map framebuffer device to memory\n");
        goto handle_ioctl_error;
    }

    // Get fixed screen information
    if (ioctl(priv->fb_fd, FBIOGET_FSCREENINFO, &priv->fb_finfo) == -1) {
        fprintf(stderr, "ERROR (screen) reading fixed information\n");
        goto handle_ioctl_error;
    }

    // Create the cairo surface which will be used to draw to
    surface = cairo_image_surface_create_for_data(
        priv->fb_data, CAIRO_FORMAT_RGB16_565, priv->fb_vinfo.xres, priv->fb_vinfo.yres,
        cairo_format_stride_for_width(CAIRO_FORMAT_RGB16_565, priv->fb_vinfo.xres));
    cairo_surface_set_user_data(surface, NULL, priv, &screen_fbdev_surface_destroy);

    return surface;

handle_ioctl_error:
    close(priv->fb_fd);
handle_allocate_error:
    free(priv);
    return NULL;
}

