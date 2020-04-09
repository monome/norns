#include <stdio.h>
#include <clock.h>
#include "clock_crow.h"

static bool clock_crow_last_time_set;
static int clock_crow_counter;
static double clock_crow_last_time;

#define CLOCK_CROW_DIV 4.0

void clock_crow_init() {
    clock_crow_counter = 0;
    clock_crow_last_time_set = false;
}

void clock_crow_handle_clock() {
    double beat_duration;
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

    double beat = clock_crow_counter / CLOCK_CROW_DIV;
    clock_update_reference_from(beat, beat_duration, CLOCK_SOURCE_CROW);
}
