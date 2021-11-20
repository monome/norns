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
    bool enabled;
    bool start_stop_sync;
    pthread_mutex_t lock;
} clock_link_shared_data;

static clock_reference_t clock_link_reference;

static void *clock_link_run(void *p) {
    (void)p;

    AbletonLink *link;
    AbletonLinkClock *clock;
    AbletonLinkSessionState *state;

    link = ableton_link_new(120);
    clock = ableton_link_clock(link);

    while (true) {
        if (pthread_mutex_trylock(&clock_link_shared_data.lock) == 0) {
            state = ableton_link_capture_app_session_state(link);

            uint64_t micros = ableton_link_clock_micros(clock);
            double link_tempo = ableton_link_session_state_tempo(state);
            bool link_playing = ableton_link_session_state_is_playing(state);

            if (clock_link_shared_data.start_stop_sync) {
                if (!clock_link_shared_data.playing && link_playing) {
                    ableton_link_session_state_request_beat_at_start_playing_time(state, 0, clock_link_shared_data.quantum);
                    clock_link_shared_data.playing = true;

                    // this will also reschedule pending sync events to beat 0
                    clock_start_from_source(CLOCK_SOURCE_LINK);
                    ableton_link_commit_app_session_state(link, state);
                } else if (clock_link_shared_data.playing && !link_playing) {
                    clock_link_shared_data.playing = false;
                    clock_stop_from_source(CLOCK_SOURCE_LINK);
                }
            }

            double link_beat = ableton_link_session_state_beat_at_time(state, micros, clock_link_shared_data.quantum);
            clock_update_source_reference(&clock_link_reference, link_beat, 60.0f / link_tempo);

            if (clock_link_shared_data.requested_tempo > 0) {
                ableton_link_session_state_set_tempo(state, clock_link_shared_data.requested_tempo, micros);
                ableton_link_commit_app_session_state(link, state);
                clock_link_shared_data.requested_tempo = 0;
            }

            ableton_link_enable(link, clock_link_shared_data.enabled);
            ableton_link_enable_start_stop_sync(link, clock_link_shared_data.start_stop_sync);

            ableton_link_session_state_destroy(state);
            pthread_mutex_unlock(&clock_link_shared_data.lock);
        }

        usleep(1000000 / 100);
    }

    return NULL;
}

void clock_link_init() {
    clock_reference_init(&clock_link_reference);
}

void clock_link_start() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    clock_link_shared_data.quantum = 4;
    clock_link_shared_data.requested_tempo = 0;
    clock_link_shared_data.enabled = false;

    pthread_create(&clock_link_thread, &attr, &clock_link_run, NULL);
}

void clock_link_join_session() {
    pthread_mutex_lock(&clock_link_shared_data.lock);
    clock_link_shared_data.enabled = true;
    pthread_mutex_unlock(&clock_link_shared_data.lock);
}

void clock_link_leave_session() {
    pthread_mutex_lock(&clock_link_shared_data.lock);
    clock_link_shared_data.enabled = false;
    pthread_mutex_unlock(&clock_link_shared_data.lock);
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

void clock_link_set_start_stop_sync(bool sync_enabled) {
    pthread_mutex_lock(&clock_link_shared_data.lock);
    clock_link_shared_data.start_stop_sync = sync_enabled;
    pthread_mutex_unlock(&clock_link_shared_data.lock);
}

double clock_link_get_beat() {
    return clock_get_reference_beat(&clock_link_reference);
}

double clock_link_get_tempo() {
    return clock_get_reference_tempo(&clock_link_reference);
}
