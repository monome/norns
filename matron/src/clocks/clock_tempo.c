#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

#include "clock.h"

static pthread_t clock_tempo_thread;

static void *clock_tempo_run(void *p) {
    (void) p;
    struct timespec req;
    float seconds = 1;

    while (true) {
        uint64_t nsec = (uint64_t) (seconds * 1000000000.0);

        req.tv_sec = nsec / 1000000000;
        req.tv_nsec = nsec % 1000000000;

        nanosleep(&req, NULL);
        clock_counter_increment();
    }

    return NULL;
}

void clock_tempo_start() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_create(&clock_tempo_thread, &attr, &clock_tempo_run, NULL);
}
