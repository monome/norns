#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdio.h>

#include "../events.h"

#include "../clocks/clock_midi.h"

#include "device.h"
#include "device_midi.h"
#include "device_midi_parser.h"

unsigned int dev_midi_port_count(const char *path) {
    int card;
    int alsa_dev;

    if (sscanf(path, "/dev/snd/midiC%dD%d", &card, &alsa_dev) < 0) {
        // TODO: Insert error message here
        return 0;
    }

    // mostly from amidi.c
    snd_ctl_t *ctl;
    char name[32];

    snd_rawmidi_info_t *info;
    int subs = 0;
    int subs_in = 0;
    int subs_out = 0;

    sprintf(name, "hw:%d", card);
    if (snd_ctl_open(&ctl, name, 0) < 0) {
        // TODO: Insert error message here
        return 0;
    }

    snd_rawmidi_info_alloca(&info);
    snd_rawmidi_info_set_device(info, alsa_dev);

    snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
    if (snd_ctl_rawmidi_info(ctl, info) >= 0) {
        subs_in = snd_rawmidi_info_get_subdevices_count(info);
    }

    snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
    if (snd_ctl_rawmidi_info(ctl, info) >= 0) {
        subs_out = snd_rawmidi_info_get_subdevices_count(info);
    }
    snd_ctl_close(ctl);

    subs = subs_in > subs_out ? subs_in : subs_out;
    return subs;
}

int dev_midi_init(void *self, unsigned int port_index, bool multiport_device) {
    struct dev_midi *midi = (struct dev_midi *)self;
    struct dev_common *base = (struct dev_common *)self;

    unsigned int alsa_card;
    unsigned int alsa_dev;
    char *alsa_name;

    sscanf(base->path, "/dev/snd/midiC%uD%u", &alsa_card, &alsa_dev);

    if (asprintf(&alsa_name, "hw:%u,%u,%u", alsa_card, alsa_dev, port_index) < 0) {
        fprintf(stderr, "failed to create alsa device name for card %d,%d\n", alsa_card, alsa_dev);
        return -1;
    }

    if (snd_rawmidi_open(&midi->handle_in, &midi->handle_out, alsa_name, 0) < 0) {
        fprintf(stderr, "failed to open alsa device %s\n", alsa_name);
        return -1;
    }

    char *name_with_port_index;
    if (multiport_device) {
        if (asprintf(&name_with_port_index, "%s %u", base->name, port_index + 1) < 0) {
            fprintf(stderr, "failed to create human-readable device name for card %d,%d,%d\n", alsa_card, alsa_dev,
                    port_index);
            return -1;
        }
        base->name = name_with_port_index;
    }

    base->start = &dev_midi_start;
    base->deinit = &dev_midi_deinit;

    return 0;
}

int dev_midi_virtual_init(void *self) {
    
    struct dev_midi *midi = (struct dev_midi *)self;
    struct dev_common *base = (struct dev_common *)self;

    if (snd_rawmidi_open(&midi->handle_in, &midi->handle_out, "virtual", 0) < 0) {
        fprintf(stderr, "failed to open alsa virtual device.\n");
        return -1;
    }

    // trigger reading
    snd_rawmidi_read(midi->handle_in, NULL, 0);

    base->start = &dev_midi_start;
    base->deinit = &dev_midi_deinit;

    return 0;
}

void dev_midi_deinit(void *self) {
    struct dev_midi *midi = (struct dev_midi *)self;
    snd_rawmidi_close(midi->handle_in);
    snd_rawmidi_close(midi->handle_out);
}

void *dev_midi_start(void *self) {
    struct dev_midi *midi = (struct dev_midi *)self;
    union event_data *ev;

    midi_parser_t *parser;
    parser = calloc(1, sizeof(midi_parser_t));
    parser->status = 0;
    midi_event_t *evt;

    ssize_t read = 0;

    do {
        unsigned char buf[256];
        int i, err, length;

        err = snd_rawmidi_read(midi->handle_in, buf, sizeof(buf));
        length = 0;
        
        for (i = 0; i < err; ++i)
            if ((buf[i] != MIDI_CMD_COMMON_CLOCK &&
                 buf[i] != MIDI_CMD_COMMON_SENSING) ||
                (buf[i] == MIDI_CMD_COMMON_CLOCK) ||
                (buf[i] == MIDI_CMD_COMMON_SENSING))
                buf[length++] = buf[i];
        
        if (length == 0)
            continue;

        read = length;

        for (i = 0; i < length; ++i) {
            evt = midi_parser_parse(parser, buf[i]);

            if (evt != NULL) {
                ev = event_data_new(EVENT_MIDI_EVENT);
                ev->midi_event.id = midi->dev.id;
                ev->midi_event.data[0] = evt->type;
                ev->midi_event.data[1] = evt->param1;
                ev->midi_event.data[2] = evt->param2;
                ev->midi_event.nbytes = parser->nr_bytes_total + 1;
                event_post(ev);
            }
        }

    } while (read > 0);

    free(parser);

    return NULL;
}

ssize_t dev_midi_send(void *self, uint8_t *data, size_t n) {
    struct dev_midi *midi = (struct dev_midi *)self;
    return snd_rawmidi_write(midi->handle_out, data, n);
}
