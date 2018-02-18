#include <stdio.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#include "events.h"

void handle_event(snd_seq_event_t *ev) {
    snd_midi_event_t *mev; // TODO: should be static
    union event_data *nev;

    nev = event_data_new(EVENT_MIDI_EVENT);
    nev->midi_event.data = malloc(3);
    int nbytes;

    snd_midi_event_new(0, &mev);

    nbytes = snd_midi_event_decode(mev, nev->midi_event.data, 3, ev);
    if (nbytes > 0) {
        event_post(nev);
    }
}

void* midi_run(void *p) {
    (void)p;

    snd_seq_t *seq;
    snd_seq_event_t *ev;
    int npfd;
    struct pollfd *pfd;

    snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
    snd_seq_set_client_name(seq, "norns");
    
    snd_seq_create_simple_port(seq, "in",
        SND_SEQ_PORT_CAP_WRITE | 
        SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);

    snd_seq_create_simple_port(seq, "out",
        SND_SEQ_PORT_CAP_READ |
        SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_APPLICATION);

    npfd = snd_seq_poll_descriptors_count(seq, POLLIN);
    pfd = (struct pollfd *) alloca(npfd * sizeof(struct pollfd));
    snd_seq_poll_descriptors(seq, pfd, npfd, POLLIN);

    while (1) {
        if (poll(pfd, npfd, -1) > 0) {
            do {
                snd_seq_event_input(seq, &ev);
                handle_event(ev);
                snd_seq_free_event(ev);
            } while (snd_seq_event_input_pending(seq, 0) > 0);            
        }
    }

    return NULL;
}

void midi_init() {
    pthread_attr_t attr;
    pthread_t pid;
    int s;

    s = pthread_attr_init(&attr);
    if(s != 0) { printf("midi_init(): error in pthread_attr_init(): %d\n", s); }
    s = pthread_create(&pid, &attr, &midi_run, NULL);
    if(s != 0) { printf("midi_init(): error in pthread_create(): %d\n", s); }
    pthread_attr_destroy(&attr);
}
