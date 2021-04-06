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

static clock_reference_t reference;
static clock_source_t clock_source;

void clock_init() {
    pthread_mutex_init(&reference.lock, NULL);

    clock_set_source(CLOCK_SOURCE_INTERNAL);
    clock_update_reference(0, 0.5);
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

double clock_get_beats_with_reference(clock_reference_t *reference) {
    pthread_mutex_lock(&(reference->lock));

    double current_time = clock_gettime_secondsf();
    double zero_beat_time = reference->last_beat_time - (reference->beat_duration * reference->beat);
    double this_beat = (current_time - zero_beat_time) / reference->beat_duration;

    pthread_mutex_unlock(&(reference->lock));

    return this_beat;
}

double clock_get_tempo() {
    pthread_mutex_lock(&reference.lock);

    double tempo = 60.0 / reference.beat_duration;

    pthread_mutex_unlock(&reference.lock);

    return tempo;
}

double clock_get_tempo_with_reference(clock_reference_t *reference) {
    pthread_mutex_lock(&(reference->lock));

    double tempo = 60.0 / reference->beat_duration;

    pthread_mutex_unlock(&(reference->lock));

    return tempo;
}

void clock_update_reference(double beats, double beat_duration) {
    pthread_mutex_lock(&reference.lock);

    double current_time = clock_gettime_secondsf();
    reference.beat_duration = beat_duration;
    reference.last_beat_time = current_time;
    reference.beat = beats;

    pthread_mutex_unlock(&reference.lock);
}

void clock_update_source_reference(clock_reference_t *reference, double beats, double beat_duration) {
    pthread_mutex_lock(&(reference->lock));

    double current_time = clock_gettime_secondsf();
    reference->beat_duration = beat_duration;
    reference->last_beat_time = current_time;
    reference->beat = beats;

    pthread_mutex_unlock(&(reference->lock));
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
