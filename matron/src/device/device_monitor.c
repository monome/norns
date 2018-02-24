/*
 * device_monitor.c
 */

#include <assert.h>
#include <errno.h>
#include <libudev.h>
#include <locale.h>
#include <poll.h>
#include <pthread.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "device.h"
#include "device_list.h"
#include "device_hid.h"
#include "device_monome.h"
#include "events.h"

//--- debug flags
// #define DEVICE_MONITOR_SHOW_DEVICE_SCANNING
// #define DEVICE_MONITOR_TRY_ALL_INPUT_DEVICES
// #define DEVICE_MONITOR_PRINT_UNHANDLED_DEVICES
// #define DEVICE_MONITOR_SHOW_USB_DEVICES
// #define DEVICE_MONITOR_SHOW_NON_USB_DEVICES

//---------------------
//--- types and defines

#define SUB_NAME_SIZE 32
#define NODE_NAME_SIZE 128
#define WATCH_TIMEOUT_MS 100

struct watch {
    // subsystem name to use as a filter on udev_monitor
    const char sub_name[NODE_NAME_SIZE];
    // regex pattern for checking the device node
    const char node_pattern[NODE_NAME_SIZE];
    // compiled regex
    regex_t node_regex;
    // udev monitor
    struct udev_monitor *mon;
};

//-------------------------
//----- static variables

// watchers
// FIXME: these names / paths are really arbitrary.
static struct watch w[DEV_TYPE_COUNT] = {
    {
        .sub_name = "tty",
        .node_pattern = "/dev/ttyUSB.*"
    },
    {
        .sub_name = "input",
        .node_pattern = "/dev/input/event.*"
    },
    {
        .sub_name = "sound",
        .node_pattern = "/dev/snd/midiC.*D.*"
    }
};

// file descriptors to watch/poll
struct pollfd pfds[DEV_TYPE_COUNT];
// thread for polling all the watched file descriptors
pthread_t watch_tid;

//--------------------------------
//--- static function declarations
static void *watch_loop(void *data);
static void handle_device(struct udev_device *dev);
static device_t check_dev_type (struct udev_device *dev);

//--------------------------------
//---- extern function definitions

void dev_monitor_init(void) {
    struct udev *udev = NULL;
    pthread_attr_t attr;
    int s;

    udev = udev_new();
    assert(udev);

    for(int i = 0; i < DEV_TYPE_COUNT; i++) {
        w[i].mon = udev_monitor_new_from_netlink(udev, "udev");
        if(w[i].mon == NULL) {
            fprintf(stderr,
                "failed to start udev_monitor for subsystem %s, pattern %s\n",
                w[i].sub_name,
                w[i].node_pattern);
            continue;
        }
        if( udev_monitor_filter_add_match_subsystem_devtype(w[i].mon,
                                                            w[i].sub_name,
                                                            NULL) < 0) {
            fprintf(stderr,
                "failed to add udev monitor filter for subsystem %s, pattern %s\n",
                w[i].sub_name,
                w[i].node_pattern);
            continue;
        }
        if (udev_monitor_enable_receiving(w[i].mon) < 0) {
            fprintf(stderr,
                "failed to enable monitor receiving for for subsystem %s, pattern %s\n",
                w[i].sub_name,
                w[i].node_pattern);
            continue;
        }

        pfds[i].fd = udev_monitor_get_fd(w[i].mon);
        pfds[i].events = POLLIN;

        if( regcomp(&w[i].node_regex, w[i].node_pattern, 0) ) {
            fprintf(stderr, "error compiling regex for device pattern: %s\n",
                   w[i].node_pattern);
        }
    } // end dev type loop
    s = pthread_attr_init(&attr);
    if (s) { fprintf(stderr, "error initializing thread attributes \n"); }
    s = pthread_create(&watch_tid, &attr, watch_loop, NULL);
    if (s) { fprintf(stderr, "error creating thread\n"); }
    pthread_attr_destroy(&attr);
}

void dev_monitor_deinit(void) {
    pthread_cancel(watch_tid);
    for(int i = 0; i < DEV_TYPE_COUNT; i++) {
        free(w[i].mon);
    }
}

int dev_monitor_scan(void) {
    struct udev *udev;
    struct udev_device *dev;
    const char *node;

    udev = udev_new();
    if (!udev) {
        fprintf(stderr, "device_monitor_scan(): failed to create udev\n");
        return 1;
    }

    for(int i = 0; i < DEV_TYPE_COUNT; i++) {
        struct udev_enumerate *ue;
        struct udev_list_entry *devices, *dev_list_entry;
#ifdef DEVICE_MONITOR_SHOW_DEVICE_SCANNING
        fprintf(stderr, "scanning for devices of type %s\n", w[i].sub_name);
#endif
        ue = udev_enumerate_new(udev);
        udev_enumerate_add_match_subsystem(ue, w[i].sub_name);
        udev_enumerate_scan_devices(ue);
        devices = udev_enumerate_get_list_entry(ue);
        udev_list_entry_foreach(dev_list_entry, devices) {
            const char *path;
            path = udev_list_entry_get_name(dev_list_entry);
#ifdef DEVICE_MONITOR_SHOW_DEVICE_SCANNING
            fprintf(stderr, "scanning with udev at path: %s\n", path);
#endif
            dev = udev_device_new_from_syspath(udev, path);
            if (dev != NULL) {
#ifdef DEVICE_MONITOR_TRY_ALL_INPUT_DEVICES
                if(1) {
#else
                if( udev_device_get_parent_with_subsystem_devtype(dev, "usb",
                                                                  NULL) ) {
#endif

                    node = udev_device_get_devnode(dev);
                    if(node != NULL) {
                        device_t t = check_dev_type(dev);
                        if( ( t >= 0) && ( t < DEV_TYPE_COUNT) ) {
#ifdef DEVICE_MONITOR_SHOW_USB_DEVICES
                            fprintf(stderr, "found usb input device; type: %d\n", t);
#endif
                            dev_list_add(t, node);
                        }
                    }
                    udev_device_unref(dev);
                }
#ifdef DEVICE_MONITOR_SHOW_NON_USB_DEVICES
                else {
                    fprintf(stderr, "found non-usb input device; type: %d \n",
                        check_dev_type(dev));
                }
#endif
            }
        }
        udev_enumerate_unref(ue);
    }

    return 0;
}

//-------------------------------
//--- static function definitions

void *watch_loop(void *p) {
    (void)p;
    struct udev_device *dev;

    while(1) {
        if (poll(pfds, DEV_TYPE_COUNT, WATCH_TIMEOUT_MS) < 0) {
            switch (errno) {
            case EINVAL:
                perror("error in poll()");
                exit(1);
            case EINTR:
            case EAGAIN:
                continue;
            }
        }

        // see which monitor has data
        for(int i = 0; i < DEV_TYPE_COUNT; i++) {
            if(pfds[i].revents & POLLIN) {
                dev = udev_monitor_receive_device(w[i].mon);
                if (dev) {
                    handle_device(dev);
                    udev_device_unref(dev);
                }
                else {
                    fprintf(stderr,
                        "no device data from receive_device(). this is an error!\n");
                }
            }
        }
    }
}

void handle_device(struct udev_device *dev) {
    device_t t = check_dev_type(dev);
    if( ( t >= 0) && ( t < DEV_TYPE_COUNT) ) {
        fprintf(stderr, "handling device, type: %d\n", t);
        const char *act = udev_device_get_action(dev);
        const char *node = udev_device_get_devnode(dev);
        if(act[0] == 'a') {
            dev_list_add(t, node);
        } else if (act[0] == 'r') {
            dev_list_remove(t, node);
        }
    }
#ifdef DEVICE_MONITOR_PRINT_UNHANDLED_DEVICES
    fprintf(stderr, "device_monitor:handle_device(): unknown device type\n");
#endif
}

device_t check_dev_type (struct udev_device *dev) {
    static char msgbuf[128];
    device_t t = DEV_TYPE_INVALID;
    const char *node = udev_device_get_devnode(dev);

    int reti;
    if(node) {
        // for now, just get USB devices.
        // eventually we might want to use this same system for GPIO, &c...
#ifdef DEVICE_MONITOR_TRY_ALL_INPUT_DEVICES
        if(1) {
#else
        if( udev_device_get_parent_with_subsystem_devtype(dev, "usb",
                                                          NULL) ) {
#endif
            for(int i = 0; i < DEV_TYPE_COUNT; i++) {
                reti = regexec(&w[i].node_regex, node, 0, NULL, 0);
                if(reti == 0) {
                    t = i;
                    break;
                }
                else if (reti == REG_NOMATCH) {
                    ;; // nothing to do
                }
                else {
                    regerror( reti, &w[i].node_regex, msgbuf, sizeof(msgbuf) );
                    fprintf(stderr, "regex match failed: %s\n", msgbuf);
                    exit(1);
                }
            }
        }
    }
    return t;
}
