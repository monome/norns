#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "events.h"

static void events_handle_error(const char* msg) {
  printf("error in events.c : %s ; code: %d", msg, SDL_GetError());
}

static void handle_user_event(SDL_Event* ev) {
  // TODO: call lua functions as appropriate... 
  //...testing...
  printf("got user event ; code: %d ; param1: 0x%08x ; param2: 0x%08x \r\n",
		 (*ev).user.code,
		 *((uint32_t*)(*ev).user.data1),
		 *((uint32_t*)(*ev).user.data2)
		 );
}

void events_init(void) {
  SDL_InitSubSystem(SDL_INIT_EVENTS);
}

void event_post(event_t code, void* data1, void* data2) {
  SDL_Event ev;
  SDL_memset(&ev, 0, sizeof(ev));
  ev.type = SDL_USEREVENT;
  ev.user.code = code;
  ev.user.data1 = data1;
  ev.user.data2 = data2;
  
  int res = SDL_PushEvent(&ev);
  if(res != 1) { events_handle_error("event_post()"); }
}

// main loop to read events!
void event_loop(void) {
  SDL_Event e;
  int quit = 0;
  
  while(!quit) {
	while(SDL_PollEvent(&e)) {
	  switch(e.type) {
	  case SDL_USEREVENT:
		handle_user_event(&e);
		break;
	  case SDL_QUIT:
		quit = 1;
	  default:
		;;
	  }
	}
  }
  SDL_Quit();
}

