#include <stdint.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#include "events.h"
#include "oracle.h"
#include "weaver.h"


//-----------------------
//--- types, variables

typedef struct {
  // nb: we have to do some evil casts from SDL_UserEvent.
  // we are basically making our own event struct.
  // these first 4 fields must match UserEvent
  Uint32 type;     
  Uint32 timestamp;
  Uint32 windowID;
  Sint32 code;
  // these are added by us. must fit in void*[2]
  uint8_t x;
  uint8_t y;
} SDL_MonomeGridEvent;

//----------------------------
//--- static function declarations

static void events_handle_error(const char* msg);
static void post_quit_event();

//---- handlers
static void handle_sdl_event(SDL_Event *e);
static void handle_user_event(SDL_Event* ev);
static void handle_grid_press(SDL_Event* ev);
static void handle_grid_lift(SDL_Event* ev);
static void handle_joy_axis(SDL_JoyAxisEvent* ja);
static void handle_joy_button(SDL_JoyButtonEvent* jb);
static void handle_buffer_report(void) ;
static void handle_engine_report(void);
static void handle_param_report(void);

//-------------------------------
//-- extern function definitions

void events_init(void) {
  SDL_InitSubSystem(SDL_INIT_EVENTS);
  SDL_InitSubSystem(SDL_INIT_JOYSTICK);
  // FIXME: we won't get m/kb events without a window...
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
	  // printf("got SDL event type %d \n", e.type);
	  if(e.type == SDL_QUIT) {
		quit = 1;
	  } else {
		handle_sdl_event(&e);
	  }
	}
  }
  SDL_Quit();
}

void event_post_monome_grid(event_t id, int x, int y) {
  union { SDL_Event ev; SDL_MonomeGridEvent mev; } u;
  SDL_memset(&u, 0, sizeof(u));
  u.ev.type - SDL_USEREVENT;
  u.ev.user.code = id;
  u.mev.x = (uint8_t) x;
  u.mev.y = (uint8_t) y;
  SDL_PushEvent(&(u.ev));
}


//------------------------------
//-- static function definitions

void post_quit_event() {
  SDL_Event ev;
  SDL_memset(&ev, 0, sizeof(ev));
  ev.type = SDL_QUIT;  
  int res = SDL_PushEvent(&ev);
}

void events_handle_error(const char* msg) {
  printf("error in events.c : %s ; code: %d", msg, SDL_GetError());
}

//---- handlers

void handle_grid_press(SDL_Event* ev) {
  SDL_MonomeGridEvent* mev = (SDL_MonomeGridEvent*)ev;
}

void handle_grid_lift(SDL_Event* ev) {
  SDL_MonomeGridEvent* mev = (SDL_MonomeGridEvent*)ev;
}

void handle_joy_axis(SDL_JoyAxisEvent* ja) {
  w_handle_stick_axis(ja->which, ja->axis, ja->value);
}

void handle_joy_button(SDL_JoyButtonEvent* jb) {
  w_handle_stick_button(jb->which, jb->button, jb->state);
}


void handle_buffer_report(void) {
  printf("handling completed buffer report\n");
  o_lock_descriptors();
  const char** p = o_get_buffer_names();
  for(int i=0; i<o_get_num_buffers(); i++) { 
   	printf("buffer %d: %s \n", i, p[i]); 
  }
  w_handle_buffer_report(o_get_num_buffers(), o_get_buffer_names());
  o_unlock_descriptors();
}

void handle_engine_report(void) {
  // FIXME: pass this to weaver
  o_lock_descriptors();
  const char** p = o_get_engine_names();
  const int n = o_get_num_engines();
  for(int i=0; i<n; i++) {
	printf("engine %d: %s \n", i, p[i]);
  }
  // TODO
  //  w_push_engine_list(p);
  o_unlock_descriptors();
}

void handle_param_report(void) {
  printf("handling completed param report \n");
  o_lock_descriptors();
  const char** p = o_get_param_names();
  for(int i=0; i<o_get_num_params(); i++) {
	printf("param %d: %s \n", i, p[i]);
  }
  // TODO
  //  w_push_param_list(p);
  o_unlock_descriptors();
}

void handle_user_event(SDL_Event* ev) {
  switch(ev->user.code) {
	
  case EVENT_EXEC_CODE:
	w_run_code(ev->user.data1);
	// free the chunk allocated by REPL
	free(ev->user.data1);
	break;
	
  case EVENT_BUFFER_REPORT:
	handle_buffer_report();
	break;
	
  case EVENT_ENGINE_REPORT:
	handle_engine_report();
	break;
	
  case EVENT_PARAM_REPORT:
	handle_param_report();
	break;

  case EVENT_GRID_PRESS:
	handle_grid_press(ev);
	break;
  case EVENT_GRID_LIFT:
	handle_grid_lift(ev);
	break;
	
	
  case EVENT_QUIT:
	post_quit_event();
	break;
  default:
	;;
  }
}


void handle_sdl_event(SDL_Event *e) {
  switch(e->type) {
  case SDL_USEREVENT:
	handle_user_event(e);
	break;
	// FIXME: SDL won't deliver m/kb without creating a window.. 
	
    // joystick
  case SDL_JOYAXISMOTION:
	handle_joy_axis(&(e->jaxis));
	break;
  case SDL_JOYBALLMOTION:          
	break;
  case SDL_JOYHATMOTION:           
	break;
  case SDL_JOYBUTTONDOWN:
	handle_joy_button(&(e->jbutton));
	break;
  case SDL_JOYBUTTONUP:
	handle_joy_button(&(e->jbutton));          
	break;
  case SDL_JOYDEVICEADDED:         
	break;
  case SDL_JOYDEVICEREMOVED:       
	break;
	
  default:
	;; // nothing to do
  }
}
