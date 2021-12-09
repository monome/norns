#pragma once

#include <stdint.h>
#include <sys/queue.h>

#include "hardware/io.h"

extern int input_setup(lachesis_io_t *io);
extern void input_destroy(lachesis_io_t *io);
