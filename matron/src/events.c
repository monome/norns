#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "events.h"

/* //...testing.. */
/* uint32_t a = 0x12345678; */
/* uint32_t b = 0xDEADBEEF; */

static void events_handle_error(const char* msg) {
  printf("error in events.c : %s ; code: %d", msg, SDL_GetError());
}

static void handle_user_event(SDL_Event* ev) {
  //...testing...
  printf("got user event ; code: %d ; param1: 0x%08x ; param2: 0x%08x \r\n",
		 (*ev).user.code,
		 *((uint32_t*)(*ev).user.data1),
		 *((uint32_t*)(*ev).user.data2)
		 );
}

void events_init(void) {
  SDL_InitSubSystem(SDL_INIT_EVENTS);
  //...testing...
  //  event_post(EVENT_TIMER, &a, &b);
}

void event_post(event_t code, void* data1, void* data2) {
  /* printf("posting user event ; code: %d ; p1: 0x%08x ; p2: 0x%08x \r\n", */
  /* 		  code, */
  /* 		  *((uint32_t*)(data1)), */
  /* 		  *((uint32_t*)(data2)) */
  /* 		  ); */
  SDL_Event ev;
  SDL_memset(&ev, 0, sizeof(ev));
  ev.type = SDL_USEREVENT;
  ev.user.code = code;
  ev.user.data1 = data1;
  ev.user.data2 = data2;
  //  printf("ev data1: %08x \r\n", *((uint32_t*)ev.user.data1) );
  
  int res = SDL_PushEvent(&ev);
  //  printf("push result: %d \r\n", res);
}

// main loop to read events!
void event_loop(void) {
  SDL_Event e;
  int quit = 0;
  
  while(!quit) {
	while(SDL_PollEvent(&e)) {

	
	  switch(e.type) {
	  case SDL_USEREVENT:

		/*  printf("got event of type %d \r\n", e.type); */
		/*  printf("as user event: code: %d ; param1: 0x%08x ; param2: 0x%08x \r\n", */
		/* e.user.code, */
		/* *((uint32_t*)e.user.data1), */
		/* *((uint32_t*)e.user.data2) */
		/* ); */
	 		
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

