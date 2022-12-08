
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "device_hid.h"
#include "events.h"

#define TEST_NULL_AND_FREE(p)                                    \
    if ((p) != NULL) {                                           \
        free(p);                                                 \
    } else {                                                     \
        fprintf(stderr, "error: double free in device_hid.c\n"); \
    }

static void add_types(struct dev_hid *d) {
    struct libevdev *dev = d->dev;
    d->types = calloc(EV_MAX, sizeof(int));
    d->num_types = 0;
    for (int i = 0; i < EV_MAX; i++) {
        if (libevdev_has_event_type(dev, i)) {
            d->types[d->num_types++] = i;
        }
    }
    d->types = realloc(d->types, d->num_types * sizeof(int));
}

static void add_codes(struct dev_hid *d) {
    struct libevdev *dev = d->dev;
    d->num_codes = calloc(d->num_types, sizeof(int));
    d->codes = calloc(d->num_types, sizeof(dev_code_t *));
    for (int i = 0; i < d->num_types; i++) {
        int max_codes, num_codes = 0;
        uint16_t *codes;
        int type = d->types[i];
        max_codes = libevdev_event_type_get_max(type);
        codes = calloc(max_codes, sizeof(dev_code_t));

        for (int code = 0; code < max_codes; code++) {
            if (libevdev_has_event_code(dev, type, code)) {
                codes[num_codes++] = code;
            }
        }

        codes = realloc(codes, num_codes * sizeof(dev_code_t));
        d->num_codes[i] = num_codes;
        d->codes[i] = codes;
    }
}

void get_guid(struct libevdev * dev, guint16 * guid) {
    guid[0] = GINT16_TO_LE(libevdev_get_id_bustype(dev));
    guid[1] = 0;
    guid[2] = GINT16_TO_LE(libevdev_get_id_vendor(dev));
    guid[3] = 0;
    guid[4] = GINT16_TO_LE(libevdev_get_id_product(dev));
    guid[5] = 0;
    guid[6] = GINT16_TO_LE(libevdev_get_id_version(dev));
    guid[7] = 0;
}

void guid_to_string(guint16 * guid, char * guidstr) {
    static const char k_rgchHexToASCII[] = "0123456789abcdef";
    int i;
    for (i = 0; i < 8; i++) {
        unsigned char c = guid[i];

        *guidstr++ = k_rgchHexToASCII[c >> 4];
        *guidstr++ = k_rgchHexToASCII[c & 0x0F];

        c = guid[i] >> 8;
        *guidstr++ = k_rgchHexToASCII[c >> 4];
        *guidstr++ = k_rgchHexToASCII[c & 0x0F];
    }
    *guidstr = '\0';
}

int dev_hid_init(void *self) {
    struct dev_hid *d = (struct dev_hid *)self;
    struct dev_common *base = (struct dev_common *)self;
    struct libevdev *dev = NULL;
    int ret = 1;
    guint16 raw_guid[16];
    int fd = open(d->base.path, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "failed to open hid device: %s\n", d->base.path);
        return -1;
    }

    ret = libevdev_new_from_fd(fd, &dev);
    if (ret < 0) {
        fprintf(stderr, "failed to init libevdev (%s)\n", strerror(-ret));
        return ret;
    }

    d->dev = dev;

    add_types(d);
    add_codes(d);

    d->vid = libevdev_get_id_vendor(dev);
    d->pid = libevdev_get_id_product(dev);
    get_guid(dev, raw_guid);
    guid_to_string(raw_guid, d->guid);

    base->start = &dev_hid_start;
    base->deinit = &dev_hid_deinit;

    return 0;
}

static void handle_event(struct dev_hid *dev, struct input_event *inev) {
    union event_data *ev = event_data_new(EVENT_HID_EVENT);
    ev->hid_event.id = dev->base.id;
    ev->hid_event.type = inev->type;
    ev->hid_event.code = inev->code;
    ev->hid_event.value = inev->value;
    event_post(ev);
}

void *dev_hid_start(void *self) {
    struct dev_hid *di = (struct dev_hid *)self;
    int rc = 1;
    do {
        struct input_event ev;
        rc = libevdev_next_event(di->dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            // dropped...
            while (rc == LIBEVDEV_READ_STATUS_SYNC) {
                rc = libevdev_next_event(di->dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
            }
            // re-synced...
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            // filter out sync and msc events
            if (!((ev.type == EV_SYN) || (ev.type == EV_MSC))) {
                handle_event(di, &ev);
            }
        }
    } while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);
    return NULL;
}

void dev_hid_deinit(void *self) {
    struct dev_hid *di = (struct dev_hid *)self;
    for (int i = 0; i < di->num_types; i++) {
        TEST_NULL_AND_FREE(di->codes[i]);
    }
    TEST_NULL_AND_FREE(di->codes);
    TEST_NULL_AND_FREE(di->num_codes);
    TEST_NULL_AND_FREE(di->types);
}
