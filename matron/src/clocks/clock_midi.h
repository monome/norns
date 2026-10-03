#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void clock_midi_init();
void clock_midi_handle_message(uint8_t message);
double clock_midi_get_beat();
double clock_midi_get_tempo();

#ifdef __cplusplus
}
#endif
