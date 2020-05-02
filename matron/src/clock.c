#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "clock.h"
#include "events.h"

#include <lauxlib.h>
#include <lua.h>

struct clock_reference_t {
    double beat;
    double beat_duration;
    double last_beat_time;
    pthread_mutex_t lock;
};

struct clock_thread_t {
    pthread_t thread;
    bool running;
    int coro_id;
};

static struct clock_reference_t reference;
static clock_source_t clock_source;

#define NUM_THREADS 100
static struct clock_thread_t clock_thread_pool[NUM_THREADS];

struct clock_thread_arg {
    int thread_index; // thread pool index
    int coro_id;
    double seconds;
};

void clock_init() {
    for (int i = 0; i < NUM_THREADS; i++) {
        clock_thread_pool[i].running = false;
    }

    pthread_mutex_init(&reference.lock, NULL);

    clock_set_source(CLOCK_SOURCE_INTERNAL);
    clock_update_reference(0, 0.5);
}

static void *clock_schedule_resume_run(void *p) {
    struct clock_thread_arg *arg = p;
    int coro_id = arg->coro_id;
    double seconds = arg->seconds;

    struct timespec req = {
        .tv_sec = (time_t)seconds,
        .tv_nsec = (long)((seconds - req.tv_sec) * 1e+9),
    };

    clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);

    union event_data *ev = event_data_new(EVENT_CLOCK_RESUME);
    ev->clock_resume.thread_id = coro_id;
    event_post(ev);

    clock_thread_pool[arg->thread_index].coro_id = -1;
    clock_thread_pool[arg->thread_index].running = false;
    free(p);

    return NULL;
}

bool clock_schedule_resume_sleep(int coro_id, double seconds) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (int i = 0; i < NUM_THREADS; i++) {
        if (!clock_thread_pool[i].running) {
            struct clock_thread_arg *arg = malloc(sizeof(struct clock_thread_arg));
            arg->thread_index = i;
            arg->coro_id = coro_id;
            arg->seconds = seconds;

            clock_thread_pool[i].running = true;
            clock_thread_pool[i].coro_id = coro_id;
            pthread_create(&clock_thread_pool[i].thread, &attr, &clock_schedule_resume_run, arg);

            return true;
        }
    }

    return false;
}

double clock_gettime_secondsf() {
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);

    return spec.tv_sec + (spec.tv_nsec / 1.0e9);
}

double clock_gettime_beats() {
    pthread_mutex_lock(&reference.lock);

    double current_time = clock_gettime_secondsf();
    double zero_beat_time = reference.last_beat_time - (reference.beat_duration * reference.beat);
    double this_beat = (current_time - zero_beat_time) / reference.beat_duration;

    pthread_mutex_unlock(&reference.lock);

    return this_beat;
}

double clock_get_tempo() {
    pthread_mutex_lock(&reference.lock);

    double tempo = 60.0 / reference.beat_duration;

    pthread_mutex_unlock(&reference.lock);

    return tempo;
}

bool clock_schedule_resume_sync(int coro_id, double beats) {
    double zero_beat_time;
    double this_beat;
    double next_beat;
    double next_beat_time;
    int next_beat_multiplier = 0;

    pthread_mutex_lock(&reference.lock);

    double current_time = clock_gettime_secondsf();
    zero_beat_time = reference.last_beat_time - (reference.beat_duration * reference.beat);
    this_beat = (current_time - zero_beat_time) / reference.beat_duration;

    do {
        next_beat_multiplier += 1;

        next_beat = (floor(this_beat / beats) + next_beat_multiplier) * beats;
        next_beat_time = zero_beat_time + (next_beat * reference.beat_duration);
    } while (next_beat_time - current_time < reference.beat_duration * beats / 2000);

    pthread_mutex_unlock(&reference.lock);

    return clock_schedule_resume_sleep(coro_id, next_beat_time - current_time);
}

void clock_update_reference(double beats, double beat_duration) {
    pthread_mutex_lock(&reference.lock);

    double current_time = clock_gettime_secondsf();
    reference.beat_duration = beat_duration;
    reference.last_beat_time = current_time;
    reference.beat = beats;

    pthread_mutex_unlock(&reference.lock);
}

void clock_update_reference_from(double beats, double beat_duration, clock_source_t source) {
    if (clock_source == source) {
        clock_update_reference(beats, beat_duration);
    }
}

void clock_start_from(clock_source_t source) {
    if (clock_source == source) {
        union event_data *ev = event_data_new(EVENT_CLOCK_START);
        event_post(ev);
    }
}

void clock_stop_from(clock_source_t source) {
    if (clock_source == source) {
        union event_data *ev = event_data_new(EVENT_CLOCK_STOP);
        event_post(ev);
    }
}

void clock_set_source(clock_source_t source) {
    clock_source = source;
}

void clock_cancel_coro(int coro_id) {
    for (int i = 0; i < NUM_THREADS; i++) {
        if (clock_thread_pool[i].coro_id == coro_id) {
            clock_cancel(i);
        }
    }
}

void clock_cancel(int index) {
    pthread_cancel(clock_thread_pool[index].thread);
    pthread_join(clock_thread_pool[index].thread, NULL);
    clock_thread_pool[index].running = false;
    clock_thread_pool[index].coro_id = -1;
}

void clock_cancel_all() {
    for (int i = 0; i < NUM_THREADS; i++) {
        clock_cancel(i);
    }
}
