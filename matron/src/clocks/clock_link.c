#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <ableton_link.h>

#include "clock.h"

static pthread_t clock_link_thread;
static struct clock_link_shared_data_t {
    double quantum;
    double requested_tempo;
    bool playing;
    pthread_mutex_t lock;
} clock_link_shared_data;

static void *clock_link_run(void *p) {
    (void)p;

    AbletonLink *link;
    AbletonLinkClock *clock;
    AbletonLinkSessionState *state;

    link = ableton_link_new(120);
    clock = ableton_link_clock(link);
    ableton_link_enable(link, true);
    ableton_link_enable_start_stop_sync(link, true);

    while (true) {
        if (pthread_mutex_trylock(&clock_link_shared_data.lock) == 0) {
            state = ableton_link_capture_audio_session_state(link);

            double link_tempo = ableton_link_session_state_tempo(state);
            uint64_t micros = ableton_link_clock_micros(clock);
            double link_beat = ableton_link_session_state_beat_at_time(state, micros, clock_link_shared_data.quantum);
            bool link_playing = ableton_link_session_state_is_playing(state);

            clock_update_reference_from(link_beat, 60.0f / link_tempo, CLOCK_SOURCE_LINK);

            if (!clock_link_shared_data.playing && link_playing) {
                clock_link_shared_data.playing = true;
                clock_start_from(CLOCK_SOURCE_LINK);
            } else if (clock_link_shared_data.playing && !link_playing) {
                clock_link_shared_data.playing = false;
                clock_stop_from(CLOCK_SOURCE_LINK);
            }

            if (clock_link_shared_data.requested_tempo > 0) {
                ableton_link_session_state_set_tempo(state, clock_link_shared_data.requested_tempo, micros);
                clock_link_shared_data.requested_tempo = 0;
                ableton_link_commit_audio_session_state(link, state);
            }

            ableton_link_session_state_destroy(state);
            pthread_mutex_unlock(&clock_link_shared_data.lock);
        }

        usleep(1000000 / 100);
    }

    return NULL;
}

void clock_link_start() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    clock_link_shared_data.quantum = 4;
    clock_link_shared_data.requested_tempo = 0;

    pthread_create(&clock_link_thread, &attr, &clock_link_run, NULL);
}

void clock_link_set_quantum(double quantum) {
    pthread_mutex_lock(&clock_link_shared_data.lock);
    clock_link_shared_data.quantum = quantum;
    pthread_mutex_unlock(&clock_link_shared_data.lock);
}

void clock_link_set_tempo(double tempo) {
    pthread_mutex_lock(&clock_link_shared_data.lock);
    clock_link_shared_data.requested_tempo = tempo;
    pthread_mutex_unlock(&clock_link_shared_data.lock);
}
