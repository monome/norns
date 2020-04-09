#include <clock.h>
#include "clock_midi.h"

static int clock_midi_counter;
static bool clock_midi_last_tick_time_set;
static double clock_midi_last_tick_time;

#define DURATION_BUFFER_LENGTH 20

static double duration_buf[DURATION_BUFFER_LENGTH] = {0};
static uint8_t beat_duration_buf_pos = 0;
static uint8_t beat_duration_buf_len = 0;

void clock_midi_init() {
    clock_midi_counter = 0;
    clock_midi_last_tick_time_set = false;
}

static void clock_midi_handle_clock() {
    double beat_duration;
    double beat_duration_buf_sum = 0;
    double beat_duration_buf_avg;
    double current_time = clock_gettime_secondsf();

    clock_midi_counter += 1;

    // TODO: handle the case when clock tick
    // are being sent after a long delay

    if (clock_midi_last_tick_time_set) {
        beat_duration = (current_time - clock_midi_last_tick_time) * 24;
        clock_midi_last_tick_time = current_time;
        clock_midi_last_tick_time_set = true;
    } else {
        beat_duration = 0.5;
        clock_midi_last_tick_time = current_time;
        clock_midi_last_tick_time_set = true;
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

    double beat = clock_midi_counter / 24.0;
    clock_update_reference_from(beat, beat_duration_buf_avg, CLOCK_SOURCE_MIDI);
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
