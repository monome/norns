#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "clock.h"
#include "clock_scheduler.h"
#include "clock_internal.h"

#define CLOCK_INTERNAL_TICKS_PER_BEAT 24

typedef struct {
    double beat_duration;
    double tick_duration;
} clock_internal_tempo_t;

static pthread_t clock_internal_thread;
static clock_reference_t clock_internal_reference;
static bool clock_internal_restarted;

static clock_internal_tempo_t clock_internal_tempo;
static pthread_mutex_t clock_internal_tempo_lock;

static void clock_internal_sleep(double seconds) {
    struct timespec ts;

    ts.tv_sec = seconds;
    ts.tv_nsec = (seconds - ts.tv_sec) * 1000000000;

    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
}

static void *clock_internal_thread_run(void *p) {
    (void)p;

    double current_time;
    double next_tick_time;

    double beat_duration;
    double tick_duration;
    double reference_beat;

    int ticks = -1;

    current_time = clock_get_system_time();
    next_tick_time = current_time;

    while (true) {
        current_time = clock_get_system_time();

        pthread_mutex_lock(&clock_internal_tempo_lock);
        beat_duration = clock_internal_tempo.beat_duration;
        tick_duration = clock_internal_tempo.tick_duration;
        pthread_mutex_unlock(&clock_internal_tempo_lock);

        clock_internal_sleep(tick_duration + (next_tick_time - current_time));

        if (clock_internal_restarted) {
            ticks = 0;
            reference_beat = 0;

            clock_update_source_reference(&clock_internal_reference, reference_beat, beat_duration);
            clock_start_from_source(CLOCK_SOURCE_INTERNAL);

            clock_internal_restarted = false;
        } else {
            ticks++;
            reference_beat = (double) ticks / CLOCK_INTERNAL_TICKS_PER_BEAT;
            clock_update_source_reference(&clock_internal_reference, reference_beat, beat_duration);
        }

        next_tick_time += tick_duration;
    }

    return NULL;
}

static void clock_internal_start() {
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_create(&clock_internal_thread, &attr, &clock_internal_thread_run, NULL);
    pthread_attr_destroy(&attr);
}

void clock_internal_init() {
    pthread_mutex_init(&clock_internal_tempo_lock, NULL);
    clock_internal_set_tempo(120);
    clock_reference_init(&clock_internal_reference);
    clock_internal_start();
}


void clock_internal_set_tempo(double bpm) {
    pthread_mutex_lock(&clock_internal_tempo_lock);

    clock_internal_tempo.beat_duration = 60.0 / bpm;
    clock_internal_tempo.tick_duration = clock_internal_tempo.beat_duration / CLOCK_INTERNAL_TICKS_PER_BEAT;

    pthread_mutex_unlock(&clock_internal_tempo_lock);

}

void clock_internal_restart() {
    clock_internal_restarted = true;
}

void clock_internal_stop() {
    clock_stop_from_source(CLOCK_SOURCE_INTERNAL);
}

double clock_internal_get_beat() {
    return clock_get_reference_beat(&clock_internal_reference);
}

double clock_internal_get_tempo() {
    return clock_get_reference_tempo(&clock_internal_reference);
}
