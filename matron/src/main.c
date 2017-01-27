#include "events.h"
#include "timers.h"


int main(int argc, char** argv) {

  // this must come first... 
  events_init();

  //...testing...
  timer_add(0, 0.25, 16);

  
  /*
// TODO (adapt from WIP sources...)
    monome_init();
	joystick_init();
	lua_glue_init();
	oracle_init();
	weaver_init();
  */

  // this blocks until quit
  event_loop();
  
  return 0;
}
