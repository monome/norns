#pragma once
/*
  repl.h
  
  this is a quick and dirty temporary REPL,
  it should be replaced by dedicated thread connecting to IPC.
 */

#include <pthread.h>

extern void repl_loop(void);
extern void repl_run_code(char* in, char* out);
