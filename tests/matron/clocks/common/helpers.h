// shared test stubs for matron clocks
// provides controllable time, events, and clock reference helpers

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdbool.h>

#include "clock.h"

// test-controlled time and beat values
double tests_set_now(double v);
double tests_set_beats(double v);
void tests_set_nanosleep_advances_time(bool v);
void tests_set_nanosleep_strict_validation(bool v);

// time stubs used by all clock implementations
double jack_client_get_current_time(void);
double clock_get_system_time(void);
double clock_get_beats(void);

// clock reference stubs used by clock_internal, clock_crow, clock_midi, clock_link
void clock_reference_init(clock_reference_t *reference);
void clock_update_source_reference(clock_reference_t *reference, double beat, double beat_duration);
double clock_get_reference_beat(clock_reference_t *reference);
double clock_get_reference_tempo(clock_reference_t *reference);

// clock control stubs used by clock_crow, clock_internal, clock_midi, clock_link
void clock_start_from_source(clock_source_t source);
void clock_stop_from_source(clock_source_t source);
void clock_reschedule_sync_events_from_source(clock_source_t source);

// pthread mutex stubs for deterministic testing
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_trylock(pthread_mutex_t *mutex);

#ifdef __cplusplus
}
#endif
