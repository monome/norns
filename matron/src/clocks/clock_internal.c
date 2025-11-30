#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "clock.h"
#include "clock_internal.h"
#include "clock_scheduler.h"

#define CLOCK_INTERNAL_TICKS_PER_BEAT 24

typedef struct {
    double beat_duration;
    double tick_duration;
} clock_internal_tempo_t;

static pthread_t clock_internal_thread;
static bool clock_internal_thread_started = false;
static clock_reference_t clock_internal_reference;
static bool clock_internal_restarted;

static clock_internal_tempo_t clock_internal_tempo;
static pthread_mutex_t clock_internal_tempo_lock;

#ifdef NORNS_TEST
// test synchronization and state variables
static bool clock_internal_threadless = false;
static uint64_t clock_internal_test_ticks = 0;
static _Atomic uint64_t clock_internal_published_ticks = 0;
static volatile bool clock_internal_thread_stop = false;
static volatile double clock_internal_last_sleep_s = 0.0;
static volatile double clock_internal_last_next_tick_time = 0.0;
static volatile double clock_internal_last_current_time = 0.0;
static volatile double clock_internal_last_tick_duration = 0.0;
#endif

static void clock_internal_sleep(double seconds) {
    struct timespec ts;

    ts.tv_sec = seconds;
    ts.tv_nsec = (seconds - ts.tv_sec) * 1000000000;

    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
}

static void *clock_internal_thread_run(void *p) {
    (void)p;

    double current_time;
    double next_tick_time;

    double beat_duration;
    double tick_duration;
    double reference_beat;

    uint64_t ticks = 0;

    current_time = clock_get_system_time();
    next_tick_time = current_time;

    while (true) {
#ifdef NORNS_TEST
        // allow tests to stop the thread loop cleanly.
        if (clock_internal_thread_stop) {
            break;
        }
#endif
        current_time = clock_get_system_time();

        pthread_mutex_lock(&clock_internal_tempo_lock);
        beat_duration = clock_internal_tempo.beat_duration;
        tick_duration = clock_internal_tempo.tick_duration;
        pthread_mutex_unlock(&clock_internal_tempo_lock);

        // compute sleep: base tick interval + drift correction.
        double raw_sleep_s = tick_duration + (next_tick_time - current_time);
        double sleep_s = raw_sleep_s < 0 ? 0 : raw_sleep_s;

#ifdef NORNS_TEST
        // expose loop internals to validate scheduling.
        clock_internal_last_current_time = current_time;
        clock_internal_last_next_tick_time = next_tick_time;
        clock_internal_last_tick_duration = tick_duration;
        // expose pre-clamp sleep value.
        clock_internal_last_sleep_s = raw_sleep_s;
#endif
        clock_internal_sleep(sleep_s);

        if (clock_internal_restarted) {
            ticks = 0;
            reference_beat = 0;

            clock_update_source_reference(&clock_internal_reference, reference_beat, beat_duration);
            clock_start_from_source(CLOCK_SOURCE_INTERNAL);

            clock_internal_restarted = false;
        } else {
            ticks++;
            reference_beat = (double)ticks / CLOCK_INTERNAL_TICKS_PER_BEAT;
            clock_update_source_reference(&clock_internal_reference, reference_beat, beat_duration);
        }

#ifdef NORNS_TEST
        // count publishes so tests can wait for progress without sleeps.
        atomic_fetch_add(&clock_internal_published_ticks, 1);
#endif

        next_tick_time += tick_duration;

        // if schedule is >1 tick behind (time jump, suspend, or jack stall),
        // skip ahead to avoid runaway catchup.
        current_time = clock_get_system_time();
        if (next_tick_time + tick_duration < current_time) {
            double lag = current_time - next_tick_time;
            // calculate skipped ticks. place
            // next_tick_time one tick after current_time to resume pacing.
            uint64_t catchup_ticks = (uint64_t)floor(lag / tick_duration);
            next_tick_time += (catchup_ticks + 1) * tick_duration;
        }
    }

    return NULL;
}

static void clock_internal_start() {
    pthread_attr_t attr;

#ifdef NORNS_TEST
    // in tests, skip background thread creation.
    if (clock_internal_threadless) {
        return;
    }
#endif

    pthread_attr_init(&attr);
    pthread_create(&clock_internal_thread, &attr, &clock_internal_thread_run, NULL);
    pthread_attr_destroy(&attr);
    clock_internal_thread_started = true;
}

void clock_internal_init() {
    pthread_mutex_init(&clock_internal_tempo_lock, NULL);
    clock_internal_set_tempo(120);
    clock_reference_init(&clock_internal_reference);
    clock_internal_start();
}

void clock_internal_set_tempo(double bpm) {
    pthread_mutex_lock(&clock_internal_tempo_lock);

    clock_internal_tempo.beat_duration = 60.0 / bpm;
    clock_internal_tempo.tick_duration = clock_internal_tempo.beat_duration / CLOCK_INTERNAL_TICKS_PER_BEAT;

    pthread_mutex_unlock(&clock_internal_tempo_lock);
}

void clock_internal_restart() {
    clock_internal_restarted = true;
}

void clock_internal_stop() {
    clock_stop_from_source(CLOCK_SOURCE_INTERNAL);
}

double clock_internal_get_beat() {
    return clock_get_reference_beat(&clock_internal_reference);
}

double clock_internal_get_tempo() {
    return clock_get_reference_tempo(&clock_internal_reference);
}

// -----------------------------------------------------------------------------
// test seams
// test-only code. compiled only with NORNS_TEST.
// -----------------------------------------------------------------------------

#ifdef NORNS_TEST
// allow disabling background thread. step one tick manually.
// expose internals and counters for test synchronization.

void clock_internal_test_enable_threadless(bool enable) {
    clock_internal_threadless = enable;
}

// step single tick without background thread.
void clock_internal_test_tick_once() {
    double beat_duration;
    double tick_duration;

    pthread_mutex_lock(&clock_internal_tempo_lock);
    beat_duration = clock_internal_tempo.beat_duration;
    tick_duration = clock_internal_tempo.tick_duration;
    pthread_mutex_unlock(&clock_internal_tempo_lock);

    (void)tick_duration; // unused in test stepping.

    if (clock_internal_restarted) {
        clock_internal_test_ticks = 0;
        double reference_beat = 0;

        clock_update_source_reference(&clock_internal_reference, reference_beat, beat_duration);
        clock_start_from_source(CLOCK_SOURCE_INTERNAL);

        clock_internal_restarted = false;
    } else {
        clock_internal_test_ticks++;
        double reference_beat = (double)clock_internal_test_ticks / CLOCK_INTERNAL_TICKS_PER_BEAT;
        clock_update_source_reference(&clock_internal_reference, reference_beat, beat_duration);
    }
    atomic_fetch_add(&clock_internal_published_ticks, 1);
}

// set internal tick counter. simulate uptime and boundary cases.
void clock_internal_test_set_ticks(uint64_t v) {
    clock_internal_test_ticks = v;
}

// read published ticks for test sync.
uint64_t clock_internal_test_get_published_ticks(void) {
    return atomic_load(&clock_internal_published_ticks);
}

// reset published tick counter.
void clock_internal_test_reset_published_ticks(void) {
    atomic_store(&clock_internal_published_ticks, 0);
}

// stop background thread. join for clean teardown.
void clock_internal_test_stop_thread(void) {
    clock_internal_thread_stop = true;
    // join thread only if started.
    // no background thread in threadless mode.
    if (clock_internal_thread_started) {
        pthread_join(clock_internal_thread, NULL);
        clock_internal_thread_started = false;
    }
    clock_internal_thread_stop = false;
}

// expose last computed sleep (seconds) before clamping.
double clock_internal_test_get_last_sleep_s(void) {
    return clock_internal_last_sleep_s;
}
// expose last scheduled next_tick_time.
double clock_internal_test_get_last_next_tick_time(void) {
    return clock_internal_last_next_tick_time;
}
// expose last sampled current_time.
double clock_internal_test_get_last_current_time(void) {
    return clock_internal_last_current_time;
}
// expose current tick_duration.
double clock_internal_test_get_last_tick_duration(void) {
    return clock_internal_last_tick_duration;
}
#endif // NORNS_TEST