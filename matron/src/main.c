#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "args.h"
#include "events.h"
#include "input.h"
#include "timers.h"

#include "oracle.h"
#include "weaver.h"

#include "m.h" // monome glue

void cleanup(void) {
  o_deinit();
  m_deinit();
  exit(0);
}

int main(int argc, char** argv) {
  args_parse(argc, argv);

  // this must come first... 
  events_init();

  // initialize timers
  timers_init();
  
  // intialize oracle (audio glue)
  o_init();

  // FIXME: we should wait here for a signal from the audio server.
  
  // initialize weaver (interpreter glue)
  w_init();  

  // initialize monome devices
  m_init();

  atexit(cleanup);
  
  // start reading input to interpreter
  input_init();

  // i/o subsystems are ready, 
  // run the user's startup routine
  w_user_startup();

  //...testing...
  //  timer_add(0, 0.25, 16);
  
  // blocks until quit
  //  printf("starting main event loop \n"); fflush(stdout);
  event_loop();
  
  //  printf("main event loop has exited \n"); fflush(stdout);
  
}
