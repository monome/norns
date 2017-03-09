#pragma once

#include <stdlib.h>

/* 
   subprocess and IO control
 */

// launch subprocesses
extern int io_init(void);

// shutdown subprocesses
extern int io_deinit(void);

extern int io_loop(void);

// send code to matron for execution (as a c string..)
extern void io_send_code(char* buf, size_t len);
