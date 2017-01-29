#include <stdio.h>

#include "events.h"
#include "repl.h"
#include "timers.h"

#include "oracle.h"
#include "weaver.h"


int main(int argc, char** argv) {
  
  // this must come first... 
  events_init();

  //...testing...
  //  timer_add(0, 0.25, 16);

  // intialize oracle (audio glue)
  o_init();
  
  // initialize weaver (interpreter glue)
  w_init();
  
  /*
  // TODO (adapt from WIP sources...)
    monome_init();
	joystick_init();
	lua_glue_init();
  */

  // this runs in its own thread
  repl_loop();
  
  // this blocks until quit...
  event_loop();
  
  printf("exiting event loop \r\n");
  
  return 0;
}
