#pragma once

#include <stdint.h>

void clock_midi_init();
void clock_midi_handle_message(uint8_t message);
