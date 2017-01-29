#include <stdint.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#include "events.h"
#include "event_handle.h"

static void events_handle_error(const char* msg) {
  printf("error in events.c : %s ; code: %d", msg, SDL_GetError());
}
void events_init(void) {
  SDL_InitSubSystem(SDL_INIT_EVENTS);
  SDL_InitSubSystem(SDL_INIT_JOYSTICK);
  if (SDL_NumJoysticks() > 0 ) {
	SDL_Joystick *joy;
	printf("found joystick, opening js0 \r\n");
	joy = SDL_JoystickOpen(0);
	if (joy) {
	  printf("js0: %s \r\n", SDL_JoystickNameForIndex(0));
	} else {
	  printf("failed to open js0\n");
	}
  }
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
  char ch;
  
  while(!quit) {
	// checks for pending SDL events
	while(SDL_PollEvent(&e)) {
	  if(e.type == SDL_QUIT) {
		quit = 1;
	  } else {
		handle_sdl_event(&e);
	  }
	}
	// we don't actually have a window,
	// so handle command line input
	// of course this blocks event queue processing
	//	ch = getchar();
	//	printf("%c\r\n", ch);
	
  }
  SDL_Quit();
}

