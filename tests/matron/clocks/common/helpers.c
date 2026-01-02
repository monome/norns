#include "helpers.h"
#include "events.h"

#include <errno.h>
#include <stdlib.h>
#include <time.h>

static double g_now = 0.0;
static double g_beats = 0.0;
static bool g_nanosleep_advances_time = true;
static bool g_nanosleep_strict_validation = false;

// -----------------------------------------------------------------------------
// test control functions - allow tests to manipulate time and beats
// -----------------------------------------------------------------------------

double tests_set_now(double v) {
    g_now = v;
    return g_now;
}

double tests_set_beats(double v) {
    g_beats = v;
    return g_beats;
}

void tests_set_nanosleep_advances_time(bool v) {
    g_nanosleep_advances_time = v;
}
void tests_set_nanosleep_strict_validation(bool v) {
    g_nanosleep_strict_validation = v;
}

// -----------------------------------------------------------------------------
// time stubs - provide controllable time to system code
// weak symbols allow test_clock_scheduler to override with atomic versions
// -----------------------------------------------------------------------------

__attribute__((weak)) double jack_client_get_current_time(void) {
    return g_now;
}

__attribute__((weak)) double clock_get_system_time(void) {
    return g_now;
}

__attribute__((weak)) double clock_get_beats(void) {
    return g_beats;
}

// -----------------------------------------------------------------------------
// clock reference stubs - shared by clock_internal, clock_crow, clock_midi, clock_link
// -----------------------------------------------------------------------------

void clock_reference_init(clock_reference_t *reference) {
    pthread_mutex_init(&(reference->lock), NULL);
    reference->beat = 0.0;
    reference->beat_duration = 0.5; // default 120 bpm
    reference->last_beat_time = g_now;
}

void clock_update_source_reference(clock_reference_t *reference, double beat, double beat_duration) {
    reference->beat = beat;
    reference->beat_duration = beat_duration;
    reference->last_beat_time = g_now;
}

// weak symbols allow tests to override with custom implementations
__attribute__((weak)) double clock_get_reference_beat(clock_reference_t *reference) {
    return reference->beat + ((g_now - reference->last_beat_time) / reference->beat_duration);
}

__attribute__((weak)) double clock_get_reference_tempo(clock_reference_t *reference) {
    return 60.0 / reference->beat_duration;
}

// -----------------------------------------------------------------------------
// clock control stubs - used by clock implementations to signal state changes
// -----------------------------------------------------------------------------

__attribute__((weak)) void clock_start_from_source(clock_source_t source) {
    (void)source;
}

__attribute__((weak)) void clock_stop_from_source(clock_source_t source) {
    (void)source;
}

// weak symbol allows test_clock_crow to override and track reschedule calls
__attribute__((weak)) void clock_reschedule_sync_events_from_source(clock_source_t source) {
    (void)source;
}

// -----------------------------------------------------------------------------
// pthread mutex stubs - no-ops for deterministic testing
// -----------------------------------------------------------------------------

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    (void)mutex;
    (void)attr;
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0; // success
}

// -----------------------------------------------------------------------------
// time sleeping stub - advance virtual time instead of real sleeping
// -----------------------------------------------------------------------------

int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain) {
    (void)clock_id;
    (void)flags;
    (void)remain;

    // guard against NULL or invalid timespec
    if (request == NULL) {
        errno = EINVAL;
        return -1;
    }

    // malformed values: either simulate OS error or no-op per test mode
    if (request->tv_sec < 0 || request->tv_nsec < 0 || request->tv_nsec >= 1000000000L) {
        if (g_nanosleep_strict_validation) {
            errno = EINVAL;
            return -1;
        }
        return 0; // ignore invalid and do not advance time
    }

    if (g_nanosleep_advances_time) {
        double seconds = (double)request->tv_sec + (double)request->tv_nsec / 1000000000.0;
        g_now += seconds;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// event system stubs - minimal implementation for tests that don't examine events
// weak symbols allow test_clock_scheduler to override and capture posted events
// -----------------------------------------------------------------------------

__attribute__((weak)) union event_data *event_data_new(event_t t) {
    union event_data *ev = (union event_data *)malloc(sizeof(union event_data));
    ev->type = t;
    return ev;
}

__attribute__((weak)) void event_post(union event_data *ev) {
    free(ev);
}
