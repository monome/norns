#include <clock.h>
#include "clock_midi.h"

static int clock_midi_counter;
static bool clock_midi_last_tick_time_set;
static double clock_midi_last_tick_time;

void clock_midi_init() {
    clock_midi_counter = 0;
    clock_midi_last_tick_time_set = false;
}

static void clock_midi_handle_clock() {
    double beat_duration;
    double current_time = clock_gettime_secondsf();

    clock_midi_counter += 1;

    // TODO: handle the case when clock tick
    // are being sent with huge intervals between them

    if (clock_midi_last_tick_time_set) {
        beat_duration = (current_time - clock_midi_last_tick_time) * 24;
        clock_midi_last_tick_time = current_time;
        clock_midi_last_tick_time_set = true;
    } else {
        beat_duration = 2.0;
        clock_midi_last_tick_time = current_time;
        clock_midi_last_tick_time_set = true;
    }

    double beat = clock_midi_counter / 24.0;
    clock_update_reference_from(beat, beat_duration, CLOCK_SOURCE_MIDI);
}

static void clock_midi_handle_start() {
    clock_midi_counter = 0;
}

void clock_midi_handle_message(uint8_t message) {
    switch (message) {
    case 0xfa:
        clock_midi_handle_start();
        break;
    case 0xf8:
        clock_midi_handle_clock();
        break;
    }
}
