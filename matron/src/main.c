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

void print_version(void);

void cleanup(void) {
    dev_monitor_deinit();
    o_deinit();
    w_deinit();

    printf("matron shutdown complete \n"); fflush(stdout);
    exit(0);
}

int main(int argc, char **argv) {
    args_parse(argc, argv);

    print_version();

    events_init(); // <-- must come first!
    timers_init();
    o_init();      // oracle (audio)

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

    ////////
    /// test: create a bunch of "simultaneous" grid events
    for (int i=0; i<8; ++i) {
      union event_data *ev = event_data_new(EVENT_GRID_KEY);
      ev->grid_key.id = 1;
      ev->grid_key.x = i;
      ev->grid_key.y = 0;
      ev->grid_key.state = 1;
      event_post(ev);	  
    }
    ////////
    
    // blocks until quit
    event_loop();
}

void print_version(void) {
    printf("MATRON\n");
    printf("norns version: %d.%d.%d\n",
           VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("git hash: %s\n\n", VERSION_HASH);
}
