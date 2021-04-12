#include "clock_midi.h"
#include <clock.h>
#include <stdio.h>

static int clock_midi_counter;
static bool clock_midi_last_tick_time_set;
static double clock_midi_last_tick_time;

#define DURATION_BUFFER_LENGTH 24

static double duration_buf[DURATION_BUFFER_LENGTH] = {0};
static uint8_t beat_duration_buf_pos = 0;
static uint8_t beat_duration_buf_len = 0;

static double mean_scale;
static double mean_sum;

void clock_midi_init() {
    clock_midi_counter = 0;
    clock_midi_last_tick_time_set = false;
    mean_sum = 0;
    mean_scale = 1;
}

static void clock_midi_handle_clock() {
    double beat_duration;
    double current_time = clock_gettime_secondsf();

    if (clock_midi_last_tick_time_set == false) {
        clock_midi_last_tick_time_set = true;
        clock_midi_last_tick_time = current_time;
    } else {
        beat_duration = (current_time - clock_midi_last_tick_time) * 24.0;

        if (beat_duration > 4) { // assume clock stopped
            clock_midi_last_tick_time = current_time;
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

            clock_midi_counter++;
            clock_midi_last_tick_time = current_time;

            double beat = clock_midi_counter / 24.0;
            clock_update_reference_from(beat, mean_sum, CLOCK_SOURCE_MIDI);
        }
    }
}

static void clock_midi_handle_start() {
    clock_midi_counter = -1;
    clock_start_from(CLOCK_SOURCE_MIDI);
}

static void clock_midi_handle_stop() {
    clock_stop_from(CLOCK_SOURCE_MIDI);
}

void clock_midi_handle_message(uint8_t message) {
    switch (message) {
    case 0xfa:
        clock_midi_handle_start();
        break;
    case 0xf8:
        clock_midi_handle_clock();
        break;
    case 0xfc:
        clock_midi_handle_stop();
        break;
    }
}
