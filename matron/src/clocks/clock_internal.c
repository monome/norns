#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "clock.h"
#include "clock_internal.h"

static pthread_t clock_internal_thread;
static bool clock_internal_thread_running;
static double interval_seconds;
static uint64_t interval_nseconds;
static double beat;

static void *clock_internal_run(void *p) {
    (void)p;
    struct timespec req;

    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &req);

        uint64_t current_time = (1000000000 * (uint64_t)req.tv_sec) + (uint64_t)req.tv_nsec;
        uint64_t new_time = current_time + interval_nseconds;

        req.tv_sec = new_time / 1000000000;
        req.tv_nsec = new_time % 1000000000;

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, NULL);

        beat += 1.0;
        clock_update_reference_from(beat, interval_seconds, CLOCK_SOURCE_INTERNAL);
    }

    return NULL;
}

void clock_internal_init() {
    clock_internal_thread_running = false;
    clock_internal_set_tempo(120);

    clock_internal_start(0.0, true);
}

void clock_internal_set_tempo(double bpm) {
    interval_seconds = 60.0 / bpm;
    interval_nseconds = (uint64_t)(interval_seconds * 1000000000);

    clock_internal_start(beat, false);
}

void clock_internal_start(double new_beat, bool transport_start) {
    pthread_attr_t attr;

    if (clock_internal_thread_running) {
        pthread_cancel(clock_internal_thread);
        pthread_join(clock_internal_thread, NULL);
    }

    beat = new_beat;
    clock_update_reference_from(beat, interval_seconds, CLOCK_SOURCE_INTERNAL);

    if (transport_start) {
        clock_start_from(CLOCK_SOURCE_INTERNAL);
    }

    pthread_attr_init(&attr);
    pthread_create(&clock_internal_thread, &attr, &clock_internal_run, NULL);
    clock_internal_thread_running = true;
    pthread_attr_destroy(&attr);
}

void clock_internal_stop() {
    clock_stop_from(CLOCK_SOURCE_INTERNAL);
}
