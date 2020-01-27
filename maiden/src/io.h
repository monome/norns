#pragma once

#include <stdlib.h>

/*
 * subprocess and IO control
 */

enum {
  IO_MATRON,
  IO_CRONE,
  IO_UI,
  IO_COUNT
};

// launch subprocesses
extern int io_init(int argc, char **argv);

// shutdown subprocesses
extern int io_deinit(void);

extern int io_loop(void);

// send code to matron for execution (as a c string)
extern void io_send_line(int sockid, char *buf);
