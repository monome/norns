#include <stdio.h>
#include <unistd.h>

#include "args.h"
#include "events.h"
#include "repl.h"
#include "timers.h"

#include "oracle.h"
#include "weaver.h"

#include "m.h" // monome glue

int main(int argc, char** argv) {
  args_parse(argc, argv);

  // this must come first... 
  events_init();
  
  // intialize oracle (audio glue)
  o_init();

  // FIXME: we should wait here for a signal from the audio server.
  
  // initialize weaver (interpreter glue)
  w_init();  

  // initialize monome devices
  m_init();

  // creates a child thread
  repl_loop();

  // i/o subsystems are ready, 
  // run the user's startup routine
  w_user_startup();

  //...testing...
  //  timer_add(0, 0.25, 16);
  
  // blocks until quit
  printf("starting main event loop \n");
  event_loop();

  printf("main event loop has exited \n");

  o_deinit();
  
  return 0;
}
