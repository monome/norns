#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "args.h"
#include "battery.h"
#include "clock.h"
#include "clocks/clock_crow.h"
#include "clocks/clock_internal.h"
#include "clocks/clock_link.h"
#include "clocks/clock_midi.h"
#include "config.h"
#include "clocks/clock_scheduler.h"
#include "device.h"
#include "device_hid.h"
#include "device_list.h"
#include "device_midi.h"
#include "device_monitor.h"
#include "device_monome.h"
#include "events.h"
#include "hello.h"
#include "i2c.h"
#include "input.h"
#include "jack_client.h"
#include "metro.h"
#include "osc.h"
#include "platform.h"
#include "screen.h"
#include "stat.h"

#include "oracle.h"
#include "weaver.h"

void print_version(void);

void cleanup(void) {
    dev_monitor_deinit();
    osc_deinit();
    o_deinit();
    w_deinit();

    config_deinit();
    i2c_deinit();
    battery_deinit();
    stat_deinit();
    jack_client_deinit();
    fprintf(stderr, "matron shutdown complete\n");
    exit(0);
}

int main(int argc, char **argv) {
    args_parse(argc, argv);

    print_version();
    init_platform();
    printf("platform: %d\n",platform());

    events_init(); // <-- must come first!
    if (config_init()) {
        fprintf(stderr, "configuration failed\n");
        return -1;
    }

    metros_init();

    battery_init();
    stat_init();
    i2c_init();
    osc_init();
    jack_client_init();
    clock_init();
    clock_internal_init();
    clock_midi_init();
    clock_crow_init();
#if HAVE_ABLETON_LINK
    clock_link_init();
    clock_link_start();
#endif
    clock_scheduler_init();

    fprintf(stderr, "init oracle...\n");
    o_init(); // oracle (audio)

    fprintf(stderr, "init weaver...\n");
    w_init(); // weaver (scripting)

    dev_list_init();
    dev_list_add(DEV_TYPE_MIDI_VIRTUAL, NULL, "virtual");

    fprintf(stderr, "init dev_monitor...\n");
    dev_monitor_init();

    // now is a good time to set our cleanup
    fprintf(stderr, "setting cleanup...\n");
    atexit(cleanup);


    fprintf(stderr, "init input...\n");
    // start reading input to interpreter
    input_init();

    
    fprintf(stderr, "running startup...\n");
    // i/o subsystems are ready; run user startup routine
    w_startup();

    // scan for connected input devices
    fprintf(stderr, "scanning devices...\n");
    dev_monitor_scan();

    // handle all resulting events, then run "post-startup"
    fprintf(stderr, "handling pending events...\n");
    event_handle_pending();

    fprintf(stderr, "running post-startup...\n");
    w_post_startup();
    
    
    // blocks until quit
    event_loop();
}

void print_version(void) {
    printf("MATRON\n");
    printf("norns version: %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("git hash: %s\n\n", VERSION_HASH);
}
