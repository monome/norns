#pragma once
/*
  repl.h
  
  this is a quick and dirty temporary REPL,
  it should be replaced by dedicated thread connecting to IPC.
 */

//#include <SDL2/SDL.h>
//extern void repl_process_sdl_key(SDL_KeyboardEvent *key);

#include <pthread.h>

extern void repl_loop(void);
extern pthread_t repl_get_pid(void);
