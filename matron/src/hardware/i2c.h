#pragma once

#include <stdint.h>

extern void i2c_init(void);

extern void i2c_hp(int level);
extern void i2c_aout(int level, int ch);
extern void i2c_ain(int level, int ch);
