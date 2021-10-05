#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "events.h"
#include "hardware/input.h"
#include "hardware/io.h"

typedef struct _input_gpio_priv {
    int fd;
    char* dev;
} input_gpio_priv_t;

typedef struct _enc_gpio_priv {
    input_gpio_priv_t base;
    int index;
} enc_gpio_priv_t;

static int input_gpio_config(matron_io_t* io, lua_State *l);
static int enc_gpio_config(matron_io_t* io, lua_State *l);
static int input_gpio_setup(matron_io_t* io);
static void input_gpio_destroy(matron_io_t* io);
static void* enc_gpio_poll(void* data);
static void* key_gpio_poll(void* data);
static int open_and_grab(const char *pathname, int flags);

input_ops_t key_gpio_ops = {
    .io_ops.name      = "keys:gpio",
    .io_ops.type      = IO_INPUT,
    .io_ops.data_size = sizeof(input_gpio_priv_t),
    .io_ops.config    = input_gpio_config,
    .io_ops.setup     = input_gpio_setup,
    .io_ops.destroy   = input_gpio_destroy,
 
    .poll = key_gpio_poll,
};

input_ops_t enc_gpio_ops = {
    .io_ops.name      = "enc:gpio",
    .io_ops.type      = IO_INPUT,
    .io_ops.data_size = sizeof(enc_gpio_priv_t),
    .io_ops.config    = enc_gpio_config,
    .io_ops.setup     = input_gpio_setup,
    .io_ops.destroy   = input_gpio_destroy,
    
    .poll = enc_gpio_poll,
};

int input_gpio_config(matron_io_t* io, lua_State *l) {
    input_gpio_priv_t* priv = io->data;

    lua_pushstring(l, "dev");
    lua_gettable(l, -2);
    if (lua_isstring(l, -1)) {
        const char *dev = lua_tostring(l, -1);
        if (!(priv->dev = malloc(strlen(dev) + 1))) {
            fprintf(stderr, "ERROR (%s) no memory\n", io->ops->name);
            lua_settop(l, 0);
            return -1;
        }
        strcpy(priv->dev, dev);
    } else {
        fprintf(stderr, "ERROR (%s) config option 'dev' should be a string\n", io->ops->name);
        lua_settop(l, 0);
        return -1;
    }
    lua_pop(l, 1);
    return 0;
}

int enc_gpio_config(matron_io_t* io, lua_State *l) {
    int err = input_gpio_config(io, l);
    if (err) {
        return err;
    }

    enc_gpio_priv_t *priv = io->data;
    lua_pushstring(l, "index");
    lua_gettable(l, -2);
    if (lua_isinteger(l, -1)) {
        priv->index = lua_tointeger(l, -1);
    } else {
        fprintf(stderr, "ERROR (%s) config option 'index' should be an integer\n", io->ops->name);
        free(priv->base.dev);
        lua_settop(l, 0);
        return -1;
    }
    lua_pop(l, 1);
    return 0;
}

int input_gpio_setup(matron_io_t* io) {
    input_gpio_priv_t *priv = io->data;
    priv->fd = open_and_grab(priv->dev, O_RDONLY);
    if (priv->fd <= 0) {
        fprintf(stderr, "ERROR (%s) device not available: %s\n", io->ops->name, priv->dev);
        return priv->fd;
    }
    return input_setup(io);
}

void input_gpio_destroy(matron_io_t *io) {
    input_gpio_priv_t *priv = io->data;
    free(priv->dev);
    input_destroy(io);
}

void* enc_gpio_poll(void* data) {
    matron_input_t* input = data;
    enc_gpio_priv_t* priv = input->io.data;

    int rd;
    unsigned int i;
    struct input_event event[64];
    int dir[3] = {1, 1, 1};
    clock_t now[3];
    clock_t prev[3];
    clock_t diff;
    prev[0] = prev[1] = prev[2] = clock();

    while (1) {
        rd = read(priv->base.fd, event, sizeof(struct input_event) * 64);
        if (rd < (int)sizeof(struct input_event)) {
            fprintf(stderr, "ERROR (enc) read error\n");
        }

        for (i = 0; i < rd / sizeof(struct input_event); i++) {
            if (event[i].type) { // make sure it's not EV_SYN == 0
                now[i] = clock();
                diff = now[i] - prev[i];
                // fprintf(stderr, "%d\t%d\t%lu\n", n, event[i].value, diff);
                prev[i] = now[i];
                if (diff > 100) { // filter out glitches
                    if (dir[i] != event[i].value &&
                        diff > 500) { // only reverse direction if there is reasonable settling time
                        dir[i] = event[i].value;
                    }
                    union event_data *ev = event_data_new(EVENT_ENC);
                    ev->enc.n = priv->index;
                    ev->enc.delta = event[i].value;
                    event_post(ev);
                }
            }
        }
    }

    return NULL;
}

void* key_gpio_poll(void* data) {
    matron_input_t* input = data;
    input_gpio_priv_t* priv = input->io.data;

    int rd;
    unsigned int i;
    struct input_event event[64];

    while (1) {
        rd = read(priv->fd, event, sizeof(struct input_event) * 64);
        if (rd < (int)sizeof(struct input_event)) {
            fprintf(stderr, "ERROR (key) read error\n");
        }

        for (i = 0; i < rd / sizeof(struct input_event); i++) {
            if (event[i].type) { // make sure it's not EV_SYN == 0
                // fprintf(stderr, "enc%d = %d\n", n, event[i].value);
                union event_data *ev = event_data_new(EVENT_KEY);
                ev->key.n = event[i].code;
                ev->key.val = event[i].value;
                event_post(ev);
            }
        }
    }

    return NULL;
}

int open_and_grab(const char *pathname, int flags) {
    int fd;
    int open_attempts = 0, ioctl_attempts = 0;
    while (open_attempts < 200) {
        fd = open(pathname, flags);
        if (fd > 0) {
            if (ioctl(fd, EVIOCGRAB, 1) == 0) {
                ioctl(fd, EVIOCGRAB, (void *)0);
                goto done;
            }
            ioctl_attempts++;
            close(fd);
        }
        open_attempts++;
        usleep(50000); // 50ms sleep * 200 = 10s fail after 10s
    };
done:
    if (open_attempts > 0) {
        fprintf(stderr, "WARN open_and_grab GPIO '%s' required %d open attempts & %d ioctl attempts\n", pathname,
                open_attempts, ioctl_attempts);
    }
    return fd;
}
