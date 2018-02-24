#pragma once

#include <alsa/asoundlib.h>

#include "device_common.h"

struct dev_midi {
    struct dev_common dev;
    snd_rawmidi_t *handle_in;
    snd_rawmidi_t *handle_out;
};

extern int dev_midi_init(void *self);
extern void dev_midi_deinit(void *self);
extern void *dev_midi_start(void *self);
