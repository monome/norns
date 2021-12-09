#pragma once

#include <stdint.h>
#include <sys/queue.h>

#include "hardware/io.h"

extern int input_setup(matron_io_t *io);
extern void input_destroy(matron_io_t *io);
