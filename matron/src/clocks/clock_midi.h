#pragma once

#include <stdint.h>

void clock_midi_init();
void clock_midi_handle_message(uint8_t message);
double clock_midi_get_beat();
double clock_midi_get_tempo();
