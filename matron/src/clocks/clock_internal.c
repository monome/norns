#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

#include "clock.h"
#include "clock_internal.h"

static pthread_t clock_internal_thread;
static double interval_seconds;
static uint64_t interval_nseconds;

static void *clock_internal_run(void *p) {
    (void) p;
    struct timespec req;
    int beat = 0;

    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &req);

        uint64_t current_time = (1000000000 * (uint64_t)req.tv_sec) + (uint64_t)req.tv_nsec;
        uint64_t new_time = current_time + interval_nseconds;

        req.tv_sec = new_time / 1000000000;
        req.tv_nsec = new_time % 1000000000;

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, NULL);

        beat += 1;
        clock_update_reference_from(beat, interval_seconds, CLOCK_SOURCE_INTERNAL);
    }

    return NULL;
}

void clock_internal_start() {
    pthread_attr_t attr;

    clock_internal_set_tempo(120);

    pthread_attr_init(&attr);
    pthread_create(&clock_internal_thread, &attr, &clock_internal_run, NULL);
}

void clock_internal_set_tempo(double bpm) {
    interval_seconds = 60.0 / bpm;
    interval_nseconds = (uint64_t) (interval_seconds * 1000000000);
}
