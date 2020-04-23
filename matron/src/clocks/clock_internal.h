#pragma once

void clock_internal_init();
void clock_internal_set_tempo(double bpm);
void clock_internal_start(double new_beat, bool transport_start);
void clock_internal_stop();
