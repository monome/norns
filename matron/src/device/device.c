#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"

#define TEST_NULL_AND_FREE(p) \
    if ((p) != NULL) {        \
        free(p);              \
    }

// start the rx thread for a device
static int dev_start(union dev *d);

union dev *dev_new(device_t type, const char *path, const char *name, bool multiport_device,
                   unsigned int midi_port_index) {
    union dev *d = calloc(1, sizeof(union dev));

    if (d == NULL) {
        return NULL;
    }

    // initialize the base class
    d->base.type = type;
    d->base.path = strdup(path);

    d->base.name = (char *)name;

    // initialize the subclass
    switch (type) {
    case DEV_TYPE_MONOME:
        if (dev_monome_init(d) < 0) {
            goto err_init;
        };
        break;
    case DEV_TYPE_HID:
        if (dev_hid_init(d) < 0) {
            goto err_init;
        }
        break;
    case DEV_TYPE_MIDI:
        if (dev_midi_init(d, midi_port_index, multiport_device) < 0) {
            goto err_init;
        }
        break;
    case DEV_TYPE_CROW:
        if (dev_crow_init(d) < 0) {
            goto err_init;
        }
        break;
    default:
        fprintf(stderr, "calling device.c:dev_new() with unkmown device type; this is an error!");
        goto err_init;
    }
    // start the thread
    dev_start(d);
    return d;

err_init:
    free(d);
    return NULL;
}

void dev_delete(union dev *d) {
    int ret;
    // fprintf(stderr, "dev_delete(): removing device %d\n", d->base.id);

    if (pthread_kill(d->base.tid, 0) == 0) {
        // device i/o thread still running
        ret = pthread_cancel(d->base.tid);
        if (ret) {
            fprintf(stderr, "dev_delete(): error in pthread_cancel(): %d\n", ret);
            exit(EXIT_FAILURE);
        }
    }

    ret = pthread_join(d->base.tid, NULL);
    if (ret) {
        fprintf(stderr, "dev_delete(): error in pthread_join(): %d\n", ret);
        exit(EXIT_FAILURE);
    }

    d->base.deinit(d);

    TEST_NULL_AND_FREE(d->base.path);
    TEST_NULL_AND_FREE(d->base.serial);
    TEST_NULL_AND_FREE(d->base.name);

    free(d);
}

int dev_start(union dev *d) {
    pthread_attr_t attr;
    int ret;

    if (d->base.start == NULL) {
        return -1;
    }

    ret = pthread_attr_init(&attr);
    if (ret) {
        fprintf(stderr, "m_init(): error on thread attributes \n");
        return -1;
    }
    ret = pthread_create(&d->base.tid, &attr, d->base.start, d);
    pthread_attr_destroy(&attr);
    if (ret) {
        fprintf(stderr, "m_init(): error creating thread\n");
        return -1;
    }
    return 0;
}

int dev_id(union dev *d) {
    return d->base.id;
}

const char *dev_serial(union dev *d) {
    return d->base.serial;
}

const char *dev_name(union dev *d) {
    return d->base.name;
}
