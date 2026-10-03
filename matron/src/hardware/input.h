#pragma once

#include <stdint.h>
#include <sys/queue.h>

#include "hardware/io.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int input_setup(matron_io_t *io);
extern void input_destroy(matron_io_t *io);

#ifdef __cplusplus
}
#endif
