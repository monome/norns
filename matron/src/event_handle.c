#include "event_handle.h"
#include "repl.h"

static void handle_user_event(SDL_Event* ev) {
  // TODO: call lua functions as appropriate... 
  //...testing...
  printf("got user event ; code: %d ; param1: 0x%08x ; param2: 0x%08x \r\n",
		 (*ev).user.code,
		 *((uint32_t*)(*ev).user.data1),
		 *((uint32_t*)(*ev).user.data2)
		 );
}

// handle josystick axis movement
static int32_t ax[6] = { 0, 0, 0, 0, 0, 0 };
void handle_joy_axis(SDL_JoyAxisEvent* ja) {
  int idx = ja->axis;
  if(idx < 6) {
	ax[idx] = ja->value;
	printf("%d\t%d\t%d\t%d\t%d\t%d\r\n",
		   ax[0], ax[1], ax[2], ax[3], ax[4], ax[5]);
  }
}

void handle_sdl_event(SDL_Event *e) {

  switch(e->type) {
  case SDL_USEREVENT:
	handle_user_event(e);
	break;

#if 0
	// NB: these won't fire unless a window exists
	// keyboard
  case SDL_KEYDOWN:
	//	repl_process_sdl_key(&(e->key));
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
