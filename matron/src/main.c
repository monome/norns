#include "events.h"
#include "timers.h"

//...testing...
// these have to be declared here so they remain in scope on Event_Poll...
// of course this is unsafe for multiple timers... 
int timer_idx;
int timer_count;
void timer_test_cb(int idx, counter_t count) {
  // timer callback will fire from separate thread
  // need to post back to event thread
  // (because lua stack is not threadsafe)
  timer_idx = idx;
  timer_count = count;
  event_post(EVENT_TIMER, &timer_idx, &timer_count);
}

int main(int argc, char** argv) {

  // this must come first... 
  events_init();

  //...testing...
  timer_add(0, &timer_test_cb, 0.25, 16);
  
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
