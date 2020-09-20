#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "events.h"
#include "watch.h"
#include "hardware/input/matron_input.h"

typedef struct _gpio_input_dev {
    int fd;
    const char* fname;
} gpio_input_dev_t;

typedef struct _gpio_enc_dev {
    gpio_input_dev_t base;
    int index;
} gpio_enc_dev_t;

static int gpio_input_setup(matron_input_t* input);
static void* gpio_enc_poll(void* data);
static void* gpio_key_poll(void* data);
static int open_and_grab(const char *pathname, int flags);

input_ops_t gpio_key_ops = {
    .data_size = sizeof(gpio_input_dev_t),
    .setup = gpio_input_setup,
    .poll = gpio_key_poll,
};

input_ops_t gpio_enc_ops = {
    .data_size = sizeof(gpio_enc_dev_t),
    .setup = gpio_input_setup,
    .poll = gpio_enc_poll,
};

int gpio_input_setup(matron_input_t* input) {
    gpio_input_dev_t* dev = input->data;
    dev->fd = open_and_grab(input->config->dev, O_RDONLY);
    if (dev->fd <= 0) {
        fprintf(stderr, "ERROR (input %s) device not available: %s\n", input->name, dev->fname);
        return dev->fd;
    }
    return 0;
}

void* gpio_enc_poll(void* data) {
    matron_input_t* input = data;
    gpio_enc_dev_t* dev = input->data;

    int rd;
    unsigned int i;
    struct input_event event[64];
    int dir[3] = {1, 1, 1};
    clock_t now[3];
    clock_t prev[3];
    clock_t diff;
    prev[0] = prev[1] = prev[2] = clock();

    while (1) {
        rd = read(dev->base.fd, event, sizeof(struct input_event) * 64);
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
                    ev->enc.n = dev->index;
                    ev->enc.delta = event[i].value;
                    event_post(ev);
                }
            }
        }
    }

    return NULL;
}

void* gpio_key_poll(void* data) {
    matron_input_t* input = data;
    gpio_input_dev_t* dev = input->data;

    int rd;
    unsigned int i;
    struct input_event event[64];

    while (1) {
        rd = read(dev->fd, event, sizeof(struct input_event) * 64);
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

                watch_key(event[i].code, event[i].value);
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
