#pragma once

#include <pthread.h>
#include <stdbool.h>

typedef enum {
    CLOCK_SOURCE_INTERNAL = 0,
    CLOCK_SOURCE_MIDI = 1,
    CLOCK_SOURCE_LINK = 2,
    CLOCK_SOURCE_CROW = 3,
} clock_source_t;

typedef struct {
    double beat;
    double beat_duration;
    double last_beat_time;
    pthread_mutex_t lock;
} clock_reference_t;

void clock_init();
void clock_update_reference(double beats, double beat_duration);
void clock_update_reference_from(double beats, double beat_duration, clock_source_t source);
void clock_start_from(clock_source_t source);
void clock_stop_from(clock_source_t source);
void clock_set_source(clock_source_t source);

double clock_gettime_beats();
double clock_gettime_secondsf();
double clock_get_tempo();
