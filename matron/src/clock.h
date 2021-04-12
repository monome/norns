#pragma once

#include <stdbool.h>

typedef enum {
    CLOCK_SOURCE_INTERNAL = 0,
    CLOCK_SOURCE_MIDI = 1,
    CLOCK_SOURCE_LINK = 2,
    CLOCK_SOURCE_CROW = 3,
} clock_source_t;

void clock_init();
bool clock_schedule_resume_sleep(int thread_id, double seconds);
bool clock_schedule_resume_sync(int thread_id, double beats);
void clock_update_reference(double beats, double beat_duration);
void clock_update_reference_from(double beats, double beat_duration, clock_source_t source);
void clock_start_from(clock_source_t source);
void clock_stop_from(clock_source_t source);
void clock_set_source(clock_source_t source);
void clock_cancel_all();

double clock_gettime_beats();
double clock_gettime_secondsf();
double clock_get_tempo();

void clock_cancel(int);
void clock_cancel_coro(int);
