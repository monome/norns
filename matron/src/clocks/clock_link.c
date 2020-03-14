#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include <ableton_link.h>

#include "clock.h"

static pthread_t clock_link_thread;

static void *clock_link_run(void *p) {
    (void) p;

    AbletonLink *link;
    AbletonLinkClock *clock;
    AbletonLinkSessionState *state;

    link = ableton_link_new(120);
    clock = ableton_link_clock(link);
    ableton_link_enable(link, true);

    int beat = 0;

    while (true) {
        state = ableton_link_capture_app_session_state(link);

        double tempo = ableton_link_session_state_tempo(state);
        long micros = ableton_link_clock_micros(clock);
        double link_beat = ableton_link_session_state_beat_at_time(state, micros, 4);
        double link_round_beat = (int) link_beat;

        if (link_round_beat != beat) {
            clock_update_counter(beat, 60.0f / tempo);
        }

        ableton_link_session_state_destroy(state);
        usleep(1000000 / 100);
    }

    return NULL;
}

void clock_link_start() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_create(&clock_link_thread, &attr, &clock_link_run, NULL);
}
