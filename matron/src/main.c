#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "args.h"
#include "events.h"
#include "input.h"
#include "timers.h"
#include "usb_monitor.h"

#include "oracle.h"
#include "weaver.h"

#include "m.h" // monome glue

void cleanup(void) {
  usb_monitor_deinit();
  m_deinit();
  o_deinit();
  printf("matron shutdown complete \n"); fflush(stdout);
  exit(0);
}

int main(int argc, char** argv) {
  args_parse(argc, argv);

  // this must come first... 
  events_init();
  
  timers_init();
  
  o_init(); // oracle


  //=== FIXME:
  //--- we should wait here for a signal from the audio server...


  w_init(); // weaver
  m_init(); // monome

  // hid devices?
  // TODO
  
  usb_monitor_init();
  //  usleep(10000000);
  
  usb_monitor_scan();
  printf("done scanning\n"); fflush(stdout);
  
  // now is a good time to set our cleanup 
  atexit(cleanup);
  
  // start reading input to interpreter
  input_init();

  // i/o subsystems are ready, 
  // run the user's startup routine
  w_user_startup();
  
  // blocks until quit
  event_loop();
  
}
