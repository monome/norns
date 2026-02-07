#pragma once

#include "hardware/io.h"

#ifndef NORNS_DESKTOP
extern input_ops_t key_gpio_ops;
extern input_ops_t enc_gpio_ops;
#else
extern input_ops_t input_sdl_ops;
#endif
