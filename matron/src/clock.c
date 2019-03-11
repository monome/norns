#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#include "events.h"
#include "clock.h"

#include <lua.h>
#include <lauxlib.h>

static int beats;
static float beat_duration;
static float last_beat_time;

static float clock_gettime_secondsf();

#define NUM_THREADS 10
static pthread_t clock_thread_pool[NUM_THREADS];
static bool clock_thread_running[NUM_THREADS];

struct thread_arg {
    int thread_index; // thread pool index
    int thread_id; // lua thread (coro id)
    float seconds;
};

void clock_init() {
    beats = 0;
    beat_duration = 0.5;
    last_beat_time = clock_gettime_secondsf();

    for (int i = 0; i < NUM_THREADS; i++) {
        clock_thread_running[i] = false;
    }
}

static void *clock_schedule_resume_run(void *p) {
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

    clock_thread_running[arg->thread_index] = false;
    free(p);

    return NULL;
}

void clock_schedule_resume(int thread_id, float seconds) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    for (int i = 0; i < NUM_THREADS; i++) {
        if (!clock_thread_running[i]) {
            struct thread_arg *arg = malloc(sizeof(struct thread_arg));
            arg->thread_index = i;
            arg->thread_id = thread_id;
            arg->seconds = seconds;

            clock_thread_running[i] = true;
            pthread_create(&clock_thread_pool[i], &attr, &clock_schedule_resume_run, arg);
            break;
        }
    }
}

static void update_counters() {
    float current_beat_time = clock_gettime_secondsf();
    beat_duration = current_beat_time - last_beat_time;
    last_beat_time = current_beat_time;
}

static float clock_gettime_secondsf() {
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);

    return spec.tv_sec + (spec.tv_nsec / 1.0e9);
}

void clock_schedule_resume_sync(int thread_id, float q) {
    float current_time = clock_gettime_secondsf();
    float zero_beat_time = last_beat_time - (beat_duration * beats);
    float this_beat = (current_time - zero_beat_time) / beat_duration;
    float next_beat = (floor(this_beat / q) + 1) * q;
    float next_beat_time = zero_beat_time + (next_beat * beat_duration);

    clock_schedule_resume(thread_id, next_beat_time - current_time);
}

int clock_counter_increment() {
    update_counters();
    return ++beats;
}

int clock_counter_get() {
    return beats;
}

void clock_counter_reset() {
    beats = 0;
}

void clock_cancel_all() {
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_cancel(clock_thread_pool[i]);
        clock_thread_running[i] = false;
    }
}