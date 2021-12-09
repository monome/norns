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
void clock_reference_init(clock_reference_t *reference);
void clock_deinit();
void clock_update_source_reference(clock_reference_t *reference, double beats, double beat_duration);
double clock_get_reference_beat(clock_reference_t *reference);
double clock_get_reference_tempo(clock_reference_t *reference);
void clock_start_from_source(clock_source_t source);
void clock_stop_from_source(clock_source_t source);
void clock_reschedule_sync_events_from_source(clock_source_t source);
void clock_set_source(clock_source_t source);

double clock_get_beats();
double clock_get_system_time();
double clock_get_tempo();
