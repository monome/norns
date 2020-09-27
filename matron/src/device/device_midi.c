#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdio.h>

#include "../events.h"

#include "../clocks/clock_midi.h"
#include "device.h"
#include "device_midi.h"

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
    printf("dev_midi_virtual_init()\n");
    
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

/*
 * prints MIDI commands, formatting them nicely
 */
/*
static void print_byte(unsigned char byte)
{
    static enum {
        STATE_UNKNOWN,
        STATE_1PARAM,
        STATE_1PARAM_CONTINUE,
        STATE_2PARAM_1,
        STATE_2PARAM_2,
        STATE_2PARAM_1_CONTINUE,
        STATE_SYSEX
    } state = STATE_UNKNOWN;
    int newline = 0;

    if (byte >= 0xf8){
        newline = 1;
    }
    else if (byte >= 0xf0) {
        newline = 1;
        switch (byte) {
        case 0xf0:
            state = STATE_SYSEX;
            break;
        case 0xf1:
        case 0xf3:
            state = STATE_1PARAM;
            break;
        case 0xf2:
            state = STATE_2PARAM_1;
            break;
        case 0xf4:
        case 0xf5:
        case 0xf6:
            state = STATE_UNKNOWN;
            break;
        case 0xf7:
            newline = state != STATE_SYSEX;
            state = STATE_UNKNOWN;
            break;
        }
    } else if (byte >= 0x80) {
        newline = 1;
        if (byte >= 0xc0 && byte <= 0xdf)
            state = STATE_1PARAM;
        else
            state = STATE_2PARAM_1;
    } else // b < 0x80 
    {
        int running_status = 0;
        newline = state == STATE_UNKNOWN;
        switch (state) {
        case STATE_1PARAM:
            state = STATE_1PARAM_CONTINUE;
            break;
        case STATE_1PARAM_CONTINUE:
            running_status = 1;
            break;
        case STATE_2PARAM_1:
            state = STATE_2PARAM_2;
            break;
        case STATE_2PARAM_2:
            state = STATE_2PARAM_1_CONTINUE;
            break;
        case STATE_2PARAM_1_CONTINUE:
            running_status = 1;
            state = STATE_2PARAM_2;
            break;
        default:
            break;
        }
        if (running_status)
            fputs("\n  ", stdout);
    }
    printf("%c%02X", newline ? '\n' : ' ', byte);
}
*/

/**< Maximum size of MIDI parameters/data (largest is SYSEX data) */
#define MIDI_PARSER_MAX_DATA_SIZE 1024

/*
 *  MIDI PARSER mostly from https://github.com/FluidSynth/fluidsynth/
 */

struct midi_event_t {
    void *paramptr;           /* Pointer parameter (for SYSEX data), size is stored to param1, param2 indicates if pointer should be freed (dynamic if TRUE) */
    unsigned int dtime;       /* Delay (ticks) between this and previous event. midi tracks. */
    unsigned int param1;      /* First parameter */
    unsigned int param2;      /* Second parameter */
    unsigned char type;       /* MIDI event type */
    unsigned char channel;    /* MIDI channel */
};

struct midi_parser_t {
    unsigned char status;           /* Identifies the type of event, that is currently received ('Noteon', 'Pitch Bend' etc). */
    unsigned char channel;          /* The channel of the event that is received (in case of a channel event) */
    unsigned int nr_bytes;          /* How many bytes have been read for the current event? */
    unsigned int nr_bytes_total;    /* How many bytes does the current event type include? */
    unsigned char data[MIDI_PARSER_MAX_DATA_SIZE]; /* The parameters or SYSEX data */
    midi_event_t event;             /* The event, that is returned to the MIDI driver. */
};

enum midi_event_type {
    /* channel messages */
    NOTE_OFF = 0x80,
    NOTE_ON = 0x90,
    KEY_PRESSURE = 0xa0,
    CONTROL_CHANGE = 0xb0,
    PROGRAM_CHANGE = 0xc0,
    CHANNEL_PRESSURE = 0xd0,
    PITCH_BEND = 0xe0,
    /* system exclusive */
    MIDI_SYSEX = 0xf0,
    /* system common - never in midi files */
    MIDI_TIME_CODE = 0xf1,
    MIDI_SONG_POSITION = 0xf2,
    MIDI_SONG_SELECT = 0xf3,
    MIDI_TUNE_REQUEST = 0xf6,
    MIDI_EOX = 0xf7,
    /* system real-time - never in midi files */
    MIDI_SYNC = 0xf8,
    MIDI_TICK = 0xf9,
    MIDI_START = 0xfa,
    MIDI_CONTINUE = 0xfb,
    MIDI_STOP = 0xfc,
    MIDI_ACTIVE_SENSING = 0xfe,
    MIDI_SYSTEM_RESET = 0xff,
    /* meta event - for midi files only */
    MIDI_META_EVENT = 0xff
};

/* Purpose:
 * Returns the length of a MIDI message. */
static int midi_event_length(unsigned char event) {
    switch(event & 0xF0)
    {
    case NOTE_OFF:
    case NOTE_ON:
    case KEY_PRESSURE:
    case CONTROL_CHANGE:
    case PITCH_BEND:
        return 3;

    case PROGRAM_CHANGE:
    case CHANNEL_PRESSURE:
        return 2;
    }

    switch(event)
    {
    case MIDI_TIME_CODE:
    case MIDI_SONG_SELECT:
    case 0xF4:
    case 0xF5:
        return 2;

    case MIDI_TUNE_REQUEST:
        return 1;

    case MIDI_SONG_POSITION:
        return 3;
    }

    return 1;
}

midi_parser_t * midi_parser_new(void) {
    midi_parser_t *parser;
    parser = calloc(1, sizeof(midi_parser_t));
    parser->status = 0;
    return parser;
}

void midi_parser_del(midi_parser_t *parser){
    if (parser != NULL);
        free(parser);
}

/**
 * Parse a MIDI stream one character at a time.
 * @param parser Parser instance
 * @param c Next character in MIDI stream
 * @return A parsed MIDI event or NULL if none.  Event is internal and should
 *   not be modified or freed and is only valid until next call to this function.
 * @internal Do not expose this function to the public API. It would allow downstream
 * apps to abuse fluidsynth as midi parser, e.g. feeding it with rawmidi and pull out
 * the needed midi information using the getter functions of fluid_midi_event_t.
 * This parser however is incomplete as it e.g. only provides a limited buffer to
 * store and process SYSEX data (i.e. doesn't allow arbitrary lengths)
 */
midi_event_t* midi_parser_parse(midi_parser_t *parser, unsigned char c) {
    midi_event_t *event;

    /* Real-time messages (0xF8-0xFF) can occur anywhere, even in the middle
     * of another message. */
    if(c >= 0xF8) {
        if(c == MIDI_SYSTEM_RESET) {
            parser->event.type = c;
            parser->status = 0; /* clear the status */
            return &parser->event;
        }

        return NULL;
    }

    /* Status byte? - If previous message not yet complete, it is discarded (re-sync). */
    if(c & 0x80) {
        /* Any status byte terminates SYSEX messages (not just 0xF7) */
        if(parser->status == MIDI_SYSEX && parser->nr_bytes > 0) {
            event = &parser->event;
            fluid_midi_event_set_sysex(event, parser->data, parser->nr_bytes,
                                       FALSE);
        }
        else {
            event = NULL;
        }

        /* Voice category message? */
        if (c < 0xF0) {
            parser->channel = c & 0x0F;
            parser->status = c & 0xF0;

            /* The event consumes x bytes of data... (subtract 1 for the status byte) */
            parser->nr_bytes_total = midi_event_length(parser->status) - 1;

             /* 0  bytes read so far */
            parser->nr_bytes = 0;
        }
        else if (c == MIDI_SYSEX) {
            parser->status = MIDI_SYSEX;
            parser->nr_bytes = 0;
        }
        else {
            parser->status = 0;    /* Discard other system messages (0xF1-0xF7) */
        }

        return event; /* Return SYSEX event or NULL */
    }

    /* Data/parameter byte */

    /* Discard data bytes for events we don't care about */
    if(parser->status == 0) {
        return NULL;
    }

    /* Max data size exceeded? (SYSEX messages only really) */
    if(parser->nr_bytes == MIDI_PARSER_MAX_DATA_SIZE) {
        parser->status = 0; /* Discard the rest of the message */
        return NULL;
    }

    /* Store next byte */
    parser->data[parser->nr_bytes++] = c;

    /* Do we still need more data to get this event complete? */
    if(parser->status == MIDI_SYSEX || parser->nr_bytes < parser->nr_bytes_total) {
        return NULL;
    }

    /* Event is complete, return it.
     * Running status byte MIDI feature is also handled here. */
    parser->event.type = parser->status;
    parser->event.channel = parser->channel;
    parser->nr_bytes = 0; /* Reset data size, in case there are additional running status messages */

    switch(parser->status) {
    case NOTE_OFF:
    case NOTE_ON:
    case KEY_PRESSURE:
    case CONTROL_CHANGE:
    case PROGRAM_CHANGE:
    case CHANNEL_PRESSURE:
        parser->event.param1 = parser->data[0]; /* For example key number */
        parser->event.param2 = parser->data[1]; /* For example velocity */
        break;

    case PITCH_BEND:
        /* Pitch-bend is transmitted with 14-bit precision. */
        parser->event.param1 = (parser->data[1] << 7) | parser->data[0];
        break;

    default: /* Unlikely */
        return NULL;
    }

    return &parser->event;
}

void *dev_midi_start(void *self) {
    struct dev_midi *midi = (struct dev_midi *)self;
    //union event_data *ev;

    midi_parser_t *parser = midi_parser_new();
    midi_event_t *evt;

    ssize_t read = 0;
    //uint8_t byte = 0;
    //uint8_t msg_buf[256];
    //uint8_t msg_pos = 0;
    //uint8_t msg_len = 0;

    int npfds;
    struct pollfd *pfds;
    npfds = 1 + snd_rawmidi_poll_descriptors_count(midi->handle_in);
    pfds = alloca(npfds * sizeof(struct pollfd));
    snd_rawmidi_poll_descriptors(midi->handle_in, &pfds[1], npfds - 1);

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
        printf("\n\n ----- %d \n", length);
        for (i = 0; i < length; ++i) {
            evt = midi_parser_parse(parser, buf[i]);

            if (evt != NULL) {
                printf("ch: %c t: %c data: %d %d", evt->channel, evt->type, evt->param1, evt->param2 );
            }
            // print_byte(buf[i]);
        }
        /*
         *
        read = snd_rawmidi_read(midi->handle_in, &byte, 1);

        if (byte >= 0xf8) {
            clock_midi_handle_message(byte);

            ev = event_data_new(EVENT_MIDI_EVENT);
            ev->midi_event.id = midi->dev.id;
            ev->midi_event.data[0] = byte;
            ev->midi_event.nbytes = 1;
            event_post(ev);
            // printf("\n");
        } else {
            if (byte >= 0xf0) {
                printf("\n");
            }
            else if (byte >= 0x80) {
                // printf("0x%x ", (unsigned char)byte);

                msg_buf[0] = byte;
                msg_pos = 1;

                switch (byte & 0xf0) {
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xe0:
                case 0xf2:
                    msg_len = 3;
                    break;
                case 0xc0:
                case 0xd0:
                    msg_len = 2;
                    break;
                case 0xf0:
                    switch (byte & 0x0f) {
                    case 0x01:
                    case 0x03:
                        msg_len = 2;
                        break;
                    case 0x07:
                        // TODO: properly handle sysex length
                        msg_len = msg_pos; // sysex end
                        break;
                    case 0x00:
                        msg_len = 0; // sysex start
                        break;
                    default:
                        msg_len = 2;
                        break;
                    }
                    break;
                default:
                    msg_len = 2;
                    break;
                }
                // printf("(msg_len %d) ", msg_len);
            } else {
                // printf("%d ", (unsigned char)byte);
                msg_buf[msg_pos] = byte;
                msg_pos += 1;
            }
            // printf("[%d] ", msg_pos);

            if (msg_pos == msg_len) {
                ev = event_data_new(EVENT_MIDI_EVENT);
                ev->midi_event.id = midi->dev.id;
                ev->midi_event.data[0] = msg_buf[0];
                ev->midi_event.data[1] = msg_len > 1 ? msg_buf[1] : 0;
                ev->midi_event.data[2] = msg_len > 2 ? msg_buf[2] : 0;
                ev->midi_event.nbytes = msg_len;
                event_post(ev);

                msg_pos = 0;
                msg_len = 0;
            }
        }
        */
    } while (read > 0);

    return NULL;
}

ssize_t dev_midi_send(void *self, uint8_t *data, size_t n) {
    struct dev_midi *midi = (struct dev_midi *)self;
    return snd_rawmidi_write(midi->handle_out, data, n);
}
