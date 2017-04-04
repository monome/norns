#pragma once

#include <stdlib.h>

/*
 * subprocess and IO control
 */

enum {
  IO_MATRON_RX,
  IO_MATRON_TX,
  IO_CRONE_RX,
  IO_CRONE_TX,
  IO_COUNT
};

// launch subprocesses
extern int io_init(int argc, char **argv);

// shutdown subprocesses
extern int io_deinit(void);

extern int io_loop(void);

// send code to matron for execution (as a c string)
extern void io_send_line(int sockid, char *buf);
