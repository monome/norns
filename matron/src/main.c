#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "args.h"

#include "device.h"
#include "device_list.h"
#include "device_input.h"
#include "device_monitor.h"
#include "device_monome.h"

#include "events.h"
#include "input.h"
#include "timers.h"

#include "oracle.h"
#include "weaver.h"

void cleanup(void) {
  dev_monitor_deinit();
  o_deinit();
  w_deinit();
  
  printf("matron shutdown complete \n"); fflush(stdout); 
  exit(0);
}

int main(int argc, char** argv) { 
  args_parse(argc, argv);
  
  events_init(); // <-- must come first!
  timers_init();  
  o_init(); // oracle (audio)
  
  //=== FIXME:
  //--- we should wait here for a signal from the audio server...

  w_init(); // weaver (scripting)
  dev_list_init();
  dev_monitor_init();
  // now is a good time to set our cleanup 
  atexit(cleanup);
  // start reading input to interpreter
  input_init();
  // i/o subsystems are ready; run user startup routine
  w_user_startup();
  // scan for connected input devices
  dev_monitor_scan();  
  // blocks until quit
  event_loop();
}
