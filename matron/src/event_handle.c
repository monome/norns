#include "events.h"
#include "event_handle.h"
#include "repl.h"
#include "oracle.h"
#include "weaver.h"

static void post_quit_event() {
  SDL_Event ev;
  SDL_memset(&ev, 0, sizeof(ev));
  ev.type = SDL_QUIT;  
  int res = SDL_PushEvent(&ev);
}


static void handle_buffer_report(void) {
  // FIXME: pass this to weaver
  printf("handling completed buffer report\n");
  o_lock_descriptors();
  const char** p = o_get_buffer_names();
  for(int i=0; i<o_get_num_buffers(); i++) {
	printf("buffer %d: %s \n", i, p[i]);
  }
  o_unlock_descriptors();
}

static void handle_engine_report(void) {
  // FIXME: pass this to weaver
  o_lock_descriptors();
  const char** p = o_get_engine_names();
  const int n = o_get_num_engines();
  for(int i=0; i<n; i++) {
	printf("engine %d: %s \n", i, p[i]);
  }
  o_unlock_descriptors();
}

static void handle_param_report(void) {
  // FIXME: pass this to weaver
  printf("handling completed param report \n");
  o_lock_descriptors();
  const char** p = o_get_param_names();
  for(int i=0; i<o_get_num_params(); i++) {
	printf("param %d: %s \n", i, p[i]);
  }
  o_unlock_descriptors();
}

static void handle_user_event(SDL_Event* ev) {
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

  case EVENT_QUIT:
	// just push SDL_Quit
	post_quit_event();
  }
}

// handle josystick axis movement
void handle_joy_axis(SDL_JoyAxisEvent* ja) {
  w_handle_stick_axis(ja->which, ja->axis, ja->value);
}

void handle_sdl_event(SDL_Event *e) {
  switch(e->type) {
  case SDL_USEREVENT:
	handle_user_event(e);
	break;

#if 0
	// these won't fire unless a window exists?
	// keyboard
  case SDL_KEYDOWN:
	break;
  case SDL_KEYUP:                  
	break;
    // mouse
  case SDL_MOUSEMOTION: 
	break;
  case SDL_MOUSEBUTTONDOWN:        
	break;
  case SDL_MOUSEBUTTONUP:          
	break;
  case SDL_MOUSEWHEEL:             
	break;
#endif
	
    // joystick
  case SDL_JOYAXISMOTION:
	handle_joy_axis(&(e->jaxis));
	break;
  case SDL_JOYBALLMOTION:          
	break;
  case SDL_JOYHATMOTION:           
	break;
  case SDL_JOYBUTTONDOWN:          
	break;
  case SDL_JOYBUTTONUP:            
	break;
  case SDL_JOYDEVICEADDED:         
	break;
  case SDL_JOYDEVICEREMOVED:       
	break;

	// game controller
  case SDL_CONTROLLERAXISMOTION: 
	break;
  case SDL_CONTROLLERBUTTONDOWN:          
	break;
  case SDL_CONTROLLERBUTTONUP:            
	break;
  case SDL_CONTROLLERDEVICEADDED:         
	break;
  case SDL_CONTROLLERDEVICEREMOVED:       
	break;
  case SDL_CONTROLLERDEVICEREMAPPED:      
	break;
  default:
	;; // nothing to do
  }
}
