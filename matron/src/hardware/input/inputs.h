#pragma once

#include "hardware/io.h"

#ifdef __cplusplus
extern "C" {
#endif

extern input_ops_t key_gpio_ops;
extern input_ops_t enc_gpio_ops;
extern input_ops_t input_sdl_ops;

#ifdef __cplusplus
}
#endif
