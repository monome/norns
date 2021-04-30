#pragma once

#include <stdint.h>

void clock_crow_init();
void clock_crow_handle_clock();
void clock_crow_in_div(int div);
double clock_crow_get_beat();
double clock_crow_get_tempo();
