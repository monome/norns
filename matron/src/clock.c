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

struct clock_counter_t {
    uint32_t beats;
    float beat_duration;
    float last_beat_time;
    pthread_mutex_t lock;
};

static struct clock_counter_t counter;

static float clock_gettime_secondsf();

#define NUM_THREADS 20
static pthread_t clock_thread_pool[NUM_THREADS];
static bool clock_thread_running[NUM_THREADS];

struct thread_arg {
    int thread_index; // thread pool index
    int coro_id;
    float seconds;
};

void clock_init() {
    counter.beats = 0;
    counter.beat_duration = 0.5;
    counter.last_beat_time = clock_gettime_secondsf();

    for (int i = 0; i < NUM_THREADS; i++) {
        clock_thread_running[i] = false;
    }

    pthread_mutex_init(&counter.lock, NULL);
}

static void *clock_schedule_resume_run(void *p) {
    struct thread_arg *arg = p;
    int coro_id = arg->coro_id;
    float seconds = arg->seconds;

    struct timespec req = {
        .tv_sec = (time_t) seconds,
        .tv_nsec = (long) ((seconds - req.tv_sec) * 1e+9),
    };

    nanosleep(&req, NULL);

    union event_data *ev = event_data_new(EVENT_CLOCK_RESUME);
    ev->clock_resume.thread_id = coro_id;
    event_post(ev);

    clock_thread_running[arg->thread_index] = false;
    free(p);

    return NULL;
}

void clock_schedule_resume_sleep(int coro_id, float seconds) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    for (int i = 0; i < NUM_THREADS; i++) {
        if (!clock_thread_running[i]) {
            struct thread_arg *arg = malloc(sizeof(struct thread_arg));
            arg->thread_index = i;
            arg->coro_id = coro_id;
            arg->seconds = seconds;

            clock_thread_running[i] = true;
            pthread_create(&clock_thread_pool[i], &attr, &clock_schedule_resume_run, arg);
            break;
        }
    }
}

float clock_gettime_secondsf() {
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);

    return spec.tv_sec + (spec.tv_nsec / 1.0e9);
}

float clock_gettime_beats() {
    pthread_mutex_lock(&counter.lock);
    float current_time = clock_gettime_secondsf();
    float zero_beat_time = counter.last_beat_time - (counter.beat_duration * counter.beats);
    float this_beat = (current_time - zero_beat_time) / counter.beat_duration;
    pthread_mutex_unlock(&counter.lock);

    return this_beat;
}

void clock_schedule_resume_sync(int coro_id, float q) {
    float current_time = clock_gettime_secondsf();

    float zero_beat_time;
    float this_beat;
    float next_beat;
    float next_beat_time;
    int next_beat_quant = 0;

    pthread_mutex_lock(&counter.lock);

    do {
        next_beat_quant += 1;
        zero_beat_time = counter.last_beat_time - (counter.beat_duration * counter.beats);
        this_beat = (current_time - zero_beat_time) / counter.beat_duration;
        next_beat = (floor(this_beat / q) + next_beat_quant) * q;
        next_beat_time = zero_beat_time + (next_beat * counter.beat_duration);
    } while (next_beat_time - current_time < counter.beat_duration * q / 2);

    pthread_mutex_unlock(&counter.lock);

    clock_schedule_resume_sleep(coro_id, next_beat_time - current_time);
}

void clock_update_counter(int beats, float beat_duration) {
    pthread_mutex_lock(&counter.lock);

    float current_time = clock_gettime_secondsf();
    counter.beat_duration = beat_duration;
    counter.last_beat_time = current_time;
    counter.beats = beats;

    pthread_mutex_unlock(&counter.lock);
}

int clock_counter_get() {
    return counter.beats;
}

void clock_counter_reset() {
    counter.beats = 0;
}

void clock_cancel_all() {
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_cancel(clock_thread_pool[i]);
        clock_thread_running[i] = false;
    }
}
