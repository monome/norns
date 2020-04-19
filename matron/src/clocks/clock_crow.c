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
static double mean_sum;
static double mean_scale;

static double crow_in_div = 4.0;

void clock_crow_in_div(int div) {
  crow_in_div = (double) div;
}

void clock_crow_init() {
    clock_crow_counter = 0;
    clock_crow_last_time_set = false;
    mean_sum = 0;
}

void clock_crow_handle_clock() {
  double beat_duration;
  double current_time = clock_gettime_secondsf();

  if(clock_crow_last_time_set == false) {
    clock_crow_last_time_set = true;
    clock_crow_last_time = current_time;
  } else { 
    beat_duration = (current_time - clock_crow_last_time) * crow_in_div;

    if(beat_duration > 4) { // assume clock stopped
      clock_crow_last_time = current_time;
    }
    else {
      if (beat_duration_buf_len < DURATION_BUFFER_LENGTH) {
        beat_duration_buf_len++;
        mean_scale = 1.0/beat_duration_buf_len;
      }

      double a = beat_duration * mean_scale;
      mean_sum = mean_sum + a;
      mean_sum = mean_sum - duration_buf[beat_duration_buf_pos];
      duration_buf[beat_duration_buf_pos] = a;

      beat_duration_buf_pos = (beat_duration_buf_pos + 1) % DURATION_BUFFER_LENGTH;

      clock_crow_counter++;
      clock_crow_last_time = current_time;

      double beat = clock_crow_counter / crow_in_div;
      clock_update_reference_from(beat, mean_sum, CLOCK_SOURCE_CROW);
    }
  }
}
