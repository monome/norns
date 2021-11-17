#include "clock_crow.h"
#include <clock.h>
#include <pthread.h>
#include <stdio.h>

static bool clock_crow_last_time_set;
static int clock_crow_counter;
static double clock_crow_last_time;
static clock_reference_t clock_crow_reference;

#define DURATION_BUFFER_LENGTH 4

static double duration_buf[DURATION_BUFFER_LENGTH] = {0};
static uint8_t beat_duration_buf_pos = 0;
static uint8_t beat_duration_buf_len = 0;
static double mean_sum;
static double mean_scale;

static double crow_in_div = 4.0;
static bool crow_in_div_changed = false;
static pthread_mutex_t crow_in_div_lock;

void clock_crow_in_div(int div) {
    pthread_mutex_lock(&crow_in_div_lock);
    crow_in_div = (double)div;
    crow_in_div_changed = true;
    pthread_mutex_unlock(&crow_in_div_lock);
}

void clock_crow_init() {
    clock_crow_counter = 0;
    clock_crow_last_time_set = false;
    mean_sum = 0;
    clock_reference_init(&clock_crow_reference);
}

void clock_crow_handle_clock() {
    double beat_duration;
    double current_time = clock_get_system_time();

    if (clock_crow_last_time_set == false) {
        clock_crow_last_time_set = true;
        clock_crow_last_time = current_time;
    } else {
        pthread_mutex_lock(&crow_in_div_lock);
        beat_duration = (current_time - clock_crow_last_time) * crow_in_div;

        if (beat_duration > 4) { // assume clock stopped
            clock_crow_last_time = current_time;
        } else {
            if (beat_duration_buf_len < DURATION_BUFFER_LENGTH) {
                beat_duration_buf_len++;
                mean_scale = 1.0 / beat_duration_buf_len;
            }

            double a = beat_duration * mean_scale;
            mean_sum = mean_sum + a;
            mean_sum = mean_sum - duration_buf[beat_duration_buf_pos];
            duration_buf[beat_duration_buf_pos] = a;

            beat_duration_buf_pos = (beat_duration_buf_pos + 1) % DURATION_BUFFER_LENGTH;

            clock_crow_counter++;
            clock_crow_last_time = current_time;

            double reference_beat = clock_crow_counter / crow_in_div;
            clock_update_source_reference(&clock_crow_reference, reference_beat, mean_sum);

            if (crow_in_div_changed) {
                clock_reschedule_sync_events_from_source(CLOCK_SOURCE_CROW);
                crow_in_div_changed = false;
            }
        }

        pthread_mutex_unlock(&crow_in_div_lock);
    }
}

double clock_crow_get_beat() {
    return clock_get_reference_beat(&clock_crow_reference);
}

double clock_crow_get_tempo() {
    return clock_get_reference_tempo(&clock_crow_reference);
}
