#include <stdio.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#include "events.h"

struct midi_t {
    snd_seq_t *seq;
    int input_port;
    int output_port;
    int npfd;
    struct pollfd *pfd;
    snd_midi_event_t *parser;
    snd_seq_event_t *sev;
} midi;

void handle_event() {
    int nbytes;
    int error;
    union event_data *nev;
    snd_seq_port_info_t *port_info;
    unsigned int port_caps;

    // TODO: handle SND_SEQ_EVENT_PORT_EXIT and SND_SEQ_EVENT_PORT_CHANGE
    if (midi.sev->type == SND_SEQ_EVENT_PORT_START) {
        snd_seq_port_info_alloca(&port_info);

        fprintf(stderr, "new midi port %i:%i\n", midi.sev->data.addr.client, midi.sev->data.addr.port);

        error = snd_seq_get_any_port_info(midi.seq,
            midi.sev->data.addr.client,
            midi.sev->data.addr.port,
            port_info);

        if (error < 0) {
            fprintf(stderr, "failed to get port information\n");
        }

        port_caps = snd_seq_port_info_get_capability(port_info);

        if ((port_caps & SND_SEQ_PORT_CAP_READ) && (port_caps & SND_SEQ_PORT_CAP_SUBS_READ)) {
            error = snd_seq_connect_from(midi.seq, midi.input_port, midi.sev->data.addr.client, midi.sev->data.addr.port);

            if (error < 0) {
                fprintf(stderr, "failed to connect to port\n");
            }
        }

        return;
    }

    // ignore sysex and subscribe events
    if (snd_seq_ev_is_subscribe_type(midi.sev) || snd_seq_ev_is_variable_type(midi.sev)) {
        fprintf(stderr, "ignoring midi event of type %d\n", midi.sev->type);
        return;
    }

    nev = event_data_new(EVENT_MIDI_EVENT);
    nbytes = snd_midi_event_decode(midi.parser, nev->midi_event.data, 3, midi.sev);

    if (nbytes > 0) {
        event_post(nev);
    }
}

void* midi_run(void *p) {
    (void)p;
    
    midi.npfd = snd_seq_poll_descriptors_count(midi.seq, POLLIN);
    midi.pfd = (struct pollfd *) alloca(midi.npfd * sizeof(struct pollfd));
    snd_seq_poll_descriptors(midi.seq, midi.pfd, midi.npfd, POLLIN);

    while (1) {
        if (poll(midi.pfd, midi.npfd, -1) > 0) {
            do {
                snd_seq_event_input(midi.seq, &midi.sev);
                handle_event(midi.sev);
                snd_seq_free_event(midi.sev);
            } while (snd_seq_event_input_pending(midi.seq, 0) > 0);
        }
    }

    return NULL;
}

void midi_init() {
    pthread_attr_t attr;
    pthread_t pid;
    int s;

    snd_seq_open(&midi.seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
    snd_seq_set_client_name(midi.seq, "norns");

    midi.input_port = snd_seq_create_simple_port(midi.seq, "in",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

    midi.output_port = snd_seq_create_simple_port(midi.seq, "out",
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

    snd_seq_connect_from(midi.seq, midi.input_port, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);

    snd_midi_event_new(0, &midi.parser);

    s = pthread_attr_init(&attr);
    if(s != 0) { fprintf(stderr, "midi_init(): error in pthread_attr_init(): %d\n", s); }
    s = pthread_create(&pid, &attr, &midi_run, NULL);
    if(s != 0) { fprintf(stderr, "midi_init(): error in pthread_create(): %d\n", s); }
    pthread_attr_destroy(&attr);
}
