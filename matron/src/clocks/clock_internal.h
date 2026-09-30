#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void clock_internal_init();
void clock_internal_set_tempo(double bpm);
void clock_internal_restart();
void clock_internal_stop();
double clock_internal_get_beat();
double clock_internal_get_tempo();

#ifdef __cplusplus
}
#endif
