#include <stdio.h>
#include <clock.h>
#include "clock_crow.h"

static bool clock_crow_last_time_set;
static int clock_crow_counter;
static double clock_crow_last_time;

#define DURATION_BUFFER_LENGTH 4

static double duration_buf[DURATION_BUFFER_LENGTH] = {0};
static uint8_t beat_duration_buf_pos = 0;
static uint8_t beat_duration_buf_len = 0;

#define CLOCK_CROW_DIV 4.0

void clock_crow_init() {
    clock_crow_counter = 0;
    clock_crow_last_time_set = false;
}

void clock_crow_handle_clock() {
    double beat_duration;
    double beat_duration_buf_sum = 0;
    double beat_duration_buf_avg;
    double current_time = clock_gettime_secondsf();

    clock_crow_counter++;

    // TODO: handle the case when clock tick
    // are being sent after a long delay

    if (clock_crow_last_time_set) {
        beat_duration = (current_time - clock_crow_last_time) * CLOCK_CROW_DIV;
        clock_crow_last_time = current_time;
        clock_crow_last_time_set = true;
    } else {
        beat_duration = 0.5;
        clock_crow_last_time = current_time;
        clock_crow_last_time_set = true;
    }

    if (beat_duration_buf_len < DURATION_BUFFER_LENGTH) {
        beat_duration_buf_len++;
    }

    duration_buf[beat_duration_buf_pos] = beat_duration;
    beat_duration_buf_pos = (beat_duration_buf_pos + 1) % DURATION_BUFFER_LENGTH;

    for (int i = 0; i < beat_duration_buf_len; i++) {
        beat_duration_buf_sum += duration_buf[i];
    }

    beat_duration_buf_avg = beat_duration_buf_sum / beat_duration_buf_len;

    double beat = clock_crow_counter / CLOCK_CROW_DIV;
    clock_update_reference_from(beat, beat_duration_buf_avg, CLOCK_SOURCE_CROW);
}
