#include <CoreMIDI/CoreMIDI.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../events.h"
#include "../clocks/clock_midi.h"
#include "device.h"
#include "device_midi.h"

#define DEV_MIDI_INPUT_BUFFER_SIZE 128

static MIDIClientRef shared_client = 0;
static pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

static MIDIClientRef get_shared_client(void) {
    pthread_mutex_lock(&client_mutex);
    if (shared_client == 0) {
        CFStringRef name = CFStringCreateWithCString(NULL, "norns", kCFStringEncodingUTF8);
        OSStatus status = MIDIClientCreate(name, NULL, NULL, &shared_client);
        CFRelease(name);
        if (status != noErr) {
            fprintf(stderr, "failed to create MIDI client: %d\n", (int)status);
            shared_client = 0;
        }
    }
    pthread_mutex_unlock(&client_mutex);
    return shared_client;
}

unsigned int dev_midi_port_count(const char *path) {
    (void)path;
    return 1;
}

static inline bool is_status_byte(uint8_t byte) {
    return (byte & 0xf0) >> 7;
}

static inline bool is_status_realtime(uint8_t byte) {
    return byte >= 0xf8;
}

static inline uint8_t midi_msg_len(uint8_t status) {
    uint8_t upper = status & 0xf0;
    switch (upper) {
    case 0x80:
    case 0x90:
    case 0xa0:
    case 0xb0:
    case 0xe0:
        return 3;
    case 0xc0:
    case 0xd0:
        return 2;
    }
    switch (status) {
    case 0xf8:
    case 0xfa:
    case 0xfb:
    case 0xfc:
    case 0xfe:
    case 0xff:
        return 1;
    case 0xf1:
    case 0xf3:
        return 2;
    case 0xf2:
        return 3;
    case 0xf6:
        return 1;
    case 0xf0:
        return 3;
    case 0xf7:
        return 1;
    }
    return 2;
}

typedef struct {
    uint8_t prior_status;
    uint8_t prior_len;
    uint8_t msg_buf[3];
    uint8_t msg_pos;
    uint8_t msg_len;
    bool msg_started;
    bool msg_sysex;
} midi_input_state_t;

static void midi_input_msg_post(midi_input_state_t *state, struct dev_midi *midi) {
    union event_data *ev = event_data_new(EVENT_MIDI_EVENT);
    ev->midi_event.id = midi->dev.id;
    ev->midi_event.nbytes = state->msg_len;
    for (uint8_t n = 0; n < state->msg_len; n++) {
        ev->midi_event.data[n] = state->msg_buf[n];
    }
    event_post(ev);
}

static void midi_input_msg_start(midi_input_state_t *state, uint8_t status) {
    state->msg_pos = 0;
    state->msg_len = midi_msg_len(status);
    state->msg_started = true;
    state->msg_buf[state->msg_pos++] = status;
    state->msg_sysex = status == 0xf0;
    if (!is_status_realtime(status)) {
        state->prior_status = status;
        state->prior_len = state->msg_len;
    }
}

static void midi_input_msg_end(midi_input_state_t *state) {
    state->msg_started = false;
    state->msg_pos = 0;
}

static void midi_input_msg_acc(midi_input_state_t *state, uint8_t byte) {
    if (!state->msg_started) {
        if (state->msg_sysex) {
            state->msg_started = true;
            state->msg_pos = 0;
            state->msg_len = 3;
        } else {
            state->msg_started = true;
            state->msg_pos = 0;
            state->msg_len = state->prior_len;
            state->msg_buf[state->msg_pos++] = state->prior_status;
        }
    }
    state->msg_buf[state->msg_pos++] = byte;
    if (byte == 0xf7) {
        state->msg_sysex = false;
        state->msg_len = state->msg_pos;
    }
}

static bool midi_input_msg_complete(midi_input_state_t *state) {
    return state->msg_started && (state->msg_len == state->msg_pos);
}

static void midi_read_proc(const MIDIPacketList *packets, void *ctx, void *src) {
    (void)src;
    struct dev_midi *midi = (struct dev_midi *)ctx;
    midi_input_state_t state = {0};

    const MIDIPacket *packet = &packets->packet[0];
    for (UInt32 i = 0; i < packets->numPackets; i++) {
        for (UInt16 j = 0; j < packet->length; j++) {
            uint8_t byte = packet->data[j];

            if (byte >= 0xf8 && midi->clock_enabled) {
                clock_midi_handle_message(byte);
            }

            if (is_status_byte(byte) && byte != 0xf7) {
                midi_input_msg_start(&state, byte);
            } else {
                midi_input_msg_acc(&state, byte);
            }

            if (midi_input_msg_complete(&state)) {
                midi_input_msg_post(&state, midi);
                midi_input_msg_end(&state);
            }
        }
        packet = MIDIPacketNext(packet);
    }
}

int dev_midi_init(void *self, unsigned int port, bool multiport) {
    struct dev_midi *midi = (struct dev_midi *)self;
    struct dev_common *base = (struct dev_common *)self;

    MIDIClientRef client = get_shared_client();
    if (client == 0) {
        return -1;
    }
    midi->client = client;

    unsigned int idx;
    if (sscanf(base->path, "coremidi:%u", &idx) != 1) {
        fprintf(stderr, "invalid coremidi path: %s\n", base->path);
        return -1;
    }

    ItemCount src_count = MIDIGetNumberOfSources();
    ItemCount dst_count = MIDIGetNumberOfDestinations();

    midi->endpoint_in = 0;
    midi->endpoint_out = 0;
    midi->port_in = 0;
    midi->port_out = 0;

    if (idx < src_count) {
        midi->endpoint_in = MIDIGetSource(idx);
    }
    if (idx < dst_count) {
        midi->endpoint_out = MIDIGetDestination(idx);
    }

    if (midi->endpoint_in == 0 && midi->endpoint_out == 0) {
        fprintf(stderr, "no valid endpoint for index %u\n", idx);
        return -1;
    }

    if (midi->endpoint_in != 0) {
        CFStringRef port_name = CFStringCreateWithFormat(NULL, NULL, CFSTR("norns_in_%u"), idx);
        OSStatus status = MIDIInputPortCreate(client, port_name, midi_read_proc, midi, &midi->port_in);
        CFRelease(port_name);
        if (status != noErr) {
            fprintf(stderr, "failed to create input port: %d\n", (int)status);
            midi->port_in = 0;
        } else {
            MIDIPortConnectSource(midi->port_in, midi->endpoint_in, NULL);
            base->start = NULL;
        }
    }

    if (midi->endpoint_out != 0) {
        CFStringRef port_name = CFStringCreateWithFormat(NULL, NULL, CFSTR("norns_out_%u"), idx);
        OSStatus status = MIDIOutputPortCreate(client, port_name, &midi->port_out);
        CFRelease(port_name);
        if (status != noErr) {
            fprintf(stderr, "failed to create output port: %d\n", (int)status);
            midi->port_out = 0;
        }
    }

    if (multiport) {
        char *name_with_port;
        if (asprintf(&name_with_port, "%s %u", base->name, port + 1) >= 0) {
            base->name = name_with_port;
        }
    }

    base->start = NULL;
    base->deinit = &dev_midi_deinit;
    midi->clock_enabled = true;

    fprintf(stderr, "CoreMIDI device opened: %s (in=%d, out=%d)\n",
            base->name,
            midi->endpoint_in != 0,
            midi->endpoint_out != 0);

    return 0;
}

int dev_midi_virtual_init(void *self) {
    struct dev_midi *midi = (struct dev_midi *)self;
    struct dev_common *base = (struct dev_common *)self;

    MIDIClientRef client = get_shared_client();
    if (client == 0) {
        return -1;
    }
    midi->client = client;

    CFStringRef name = CFStringCreateWithCString(NULL, "norns virtual", kCFStringEncodingUTF8);

    OSStatus status = MIDISourceCreate(client, name, &midi->endpoint_out);
    if (status != noErr) {
        fprintf(stderr, "failed to create virtual source: %d\n", (int)status);
        CFRelease(name);
        return -1;
    }

    status = MIDIDestinationCreate(client, name, midi_read_proc, midi, &midi->endpoint_in);
    CFRelease(name);
    if (status != noErr) {
        fprintf(stderr, "failed to create virtual destination: %d\n", (int)status);
        return -1;
    }

    midi->port_in = 0;
    midi->port_out = 0;

    base->start = NULL;
    base->deinit = &dev_midi_deinit;
    midi->clock_enabled = true;

    return 0;
}

void dev_midi_deinit(void *self) {
    struct dev_midi *midi = (struct dev_midi *)self;

    if (midi->port_in != 0) {
        if (midi->endpoint_in != 0) {
            MIDIPortDisconnectSource(midi->port_in, midi->endpoint_in);
        }
        MIDIPortDispose(midi->port_in);
    }
    if (midi->port_out != 0) {
        MIDIPortDispose(midi->port_out);
    }
}

void *dev_midi_start(void *self) {
    (void)self;
    return NULL;
}

ssize_t dev_midi_send(void *self, uint8_t *data, size_t n) {
    struct dev_midi *midi = (struct dev_midi *)self;

    if (midi->endpoint_out == 0) {
        return -1;
    }

    Byte buffer[256];
    MIDIPacketList *packets = (MIDIPacketList *)buffer;
    MIDIPacket *packet = MIDIPacketListInit(packets);
    packet = MIDIPacketListAdd(packets, sizeof(buffer), packet, 0, n, data);

    if (packet == NULL) {
        return -1;
    }

    OSStatus status;
    if (midi->port_out != 0) {
        status = MIDISend(midi->port_out, midi->endpoint_out, packets);
    } else {
        status = MIDIReceived(midi->endpoint_out, packets);
    }

    return status == noErr ? (ssize_t)n : -1;
}
