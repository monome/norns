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
#include "device.h"
#include "device_hid.h"
#include "device_list.h"
#include "device_midi.h"
#include "device_monitor.h"
#include "device_monome.h"
#include "events.h"
#include "gpio.h"
#include "hello.h"
#include "i2c.h"
#include "input.h"
#include "metro.h"
#include "osc.h"
#include "screen.h"
#include "stat.h"
#include "watch.h"

#include "oracle.h"
#include "weaver.h"

void print_version(void);

void cleanup(void) {
    dev_monitor_deinit();
    osc_deinit();
    o_deinit();
    w_deinit();
    gpio_deinit();
    i2c_deinit();
    screen_deinit();
    battery_deinit();
    stat_deinit();
    watch_deinit();

    fprintf(stderr, "matron shutdown complete\n");
    exit(0);
}

int main(int argc, char **argv) {
    args_parse(argc, argv);

    print_version();

    events_init(); // <-- must come first!
    screen_init();

    metros_init();

#ifdef __arm__
    // gpio_init() hangs for too long when cross-compiling norns
    // desktop for dev - just disable on x86 for now
    gpio_init();
#endif

    battery_init();
    stat_init();
    i2c_init();
    osc_init();
    clock_init();
    clock_internal_init();
    clock_midi_init();
    clock_crow_init();
#if HAVE_ABLETON_LINK
    clock_link_start();
#endif

    watch_init();

    o_init(); // oracle (audio)

    w_init(); // weaver (scripting)
    dev_list_init();
    dev_monitor_init();

    // now is a good time to set our cleanup
    atexit(cleanup);
    // start reading input to interpreter
    input_init();
    // i/o subsystems are ready; run user startup routine
    w_startup();
    // scan for connected input devices
    dev_monitor_scan();

    // blocks until quit
    event_loop();
}

void print_version(void) {
    printf("MATRON\n");
    printf("norns version: %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("git hash: %s\n\n", VERSION_HASH);
}
