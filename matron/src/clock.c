#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#include "events.h"
#include "clock.h"

#include <lua.h>
#include <lauxlib.h>

#define NUM_THREADS 10
static pthread_t clock_threads[NUM_THREADS];
static bool clock_threads_running[NUM_THREADS];

struct thread_arg {
    int thread_index;
    int thread_id;
    float seconds;
};

void clock_init() {
    for (int i = 0; i < NUM_THREADS; i++) {
        clock_threads_running[i] = false;
    }
}

static void *clock_schedule_sleeper(void *p) {
    struct thread_arg *arg = p;
    int thread_id = arg->thread_id;
    float seconds = arg->seconds;

    struct timespec req;

    uint64_t nsec = (uint64_t) (seconds * 1000000000.0);

    req.tv_sec = nsec / 1000000000;
    req.tv_nsec = nsec % 1000000000;

    nanosleep(&req, NULL);

    union event_data *ev = event_data_new(EVENT_CLOCK_RESUME);
    ev->clock_resume.thread_id = thread_id;
    event_post(ev);

    clock_threads_running[arg->thread_index] = false;
    free(p);

    return NULL;
}

void clock_schedule(int thread_id, float seconds) {
    (void)thread_id;
    (void)seconds;

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    for (int i = 0; i < NUM_THREADS; i++) {
        if (!clock_threads_running[i]) {
            struct thread_arg *arg = malloc(sizeof(struct thread_arg));
            arg->thread_index = i;
            arg->thread_id = thread_id;
            arg->seconds = seconds;

            clock_threads_running[i] = true;
            pthread_create(&clock_threads[i], &attr, &clock_schedule_sleeper, arg);
            break;
        }
    }
}
