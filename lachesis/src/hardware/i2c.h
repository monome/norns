#pragma once

#include <stdint.h>

extern void i2c_init(void);
extern void i2c_deinit(void);

extern void i2c_hp(int level);
