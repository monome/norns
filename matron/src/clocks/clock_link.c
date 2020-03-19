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
static struct clock_link_data_t {
	double quantum;
    pthread_mutex_t lock;
} clock_link_data;

static void *clock_link_run(void *p) {
    (void) p;

    AbletonLink *link;
    AbletonLinkClock *clock;
    AbletonLinkSessionState *state;

    link = ableton_link_new(120);
    clock = ableton_link_clock(link);
    ableton_link_enable(link, true);

    while (true) {
        state = ableton_link_capture_audio_session_state(link);

        double tempo = ableton_link_session_state_tempo(state);
        long micros = ableton_link_clock_micros(clock);
        double link_beat = ableton_link_session_state_beat_at_time(state, micros, clock_link_data.quantum);

        clock_update_reference_from(link_beat, 60.0f / tempo, CLOCK_SOURCE_LINK);

        ableton_link_session_state_destroy(state);
        usleep(1000000 / 100);
    }

    return NULL;
}

void clock_link_start() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    clock_link_data.quantum = 4;

    pthread_create(&clock_link_thread, &attr, &clock_link_run, NULL);
}

void clock_link_set_quantum(double quantum) {
    pthread_mutex_lock(&clock_link_data.lock);
    clock_link_data.quantum = quantum;
    pthread_mutex_unlock(&clock_link_data.lock);
}