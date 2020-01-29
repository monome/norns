#pragma once

#include <alsa/asoundlib.h>

#include "device_common.h"

struct dev_midi {
    struct dev_common dev;
    snd_rawmidi_t *handle_in;
    snd_rawmidi_t *handle_out;
};

extern unsigned int dev_midi_port_count(const char *path);
extern int dev_midi_init(void *self, unsigned int port_index, bool multiport_device);
extern void dev_midi_deinit(void *self);
extern void* dev_midi_start(void *self);
extern ssize_t dev_midi_send(void *self, uint8_t *data, size_t n);
