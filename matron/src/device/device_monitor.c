/*
 * device_monitor.c
 */

#include <assert.h>
#include <errno.h>
#include <libudev.h>
#include <locale.h>
#include <poll.h>
#include <pthread.h>
#include <fnmatch.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "device.h"
#include "device_list.h"
#include "device_hid.h"
#include "device_monome.h"
#include "device_crow.h"
#include "events.h"

#define SUB_NAME_SIZE 32
#define NODE_NAME_SIZE 128
#define WATCH_TIMEOUT_MS 100

struct watch {
    // subsystem name to use as a filter on udev_monitor
    const char sub_name[NODE_NAME_SIZE];
    // glob pattern for checking the device node
    const char node_pattern[NODE_NAME_SIZE];
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
        .node_pattern = "/dev/ttyUSB*"
    },
    {
        .sub_name = "input",
        .node_pattern = "/dev/input/event*"
    },
    {
        .sub_name = "sound",
        .node_pattern = "/dev/snd/midiC*D*"
    },
    {
        .sub_name = "crow",
        .node_pattern = "/dev/ttyACM*"
    }
};

// file descriptors to watch/poll
struct pollfd pfds[DEV_TYPE_COUNT];
// thread for polling all the watched file descriptors
pthread_t watch_tid;

//--------------------------------
//--- static function declarations
static void* watch_loop(void *data);
static void handle_device(struct udev_device *dev);
static device_t check_dev_type(struct udev_device *dev);
static const char* get_alsa_midi_node(struct udev_device *dev);
static const char* get_device_name(struct udev_device *dev);

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
        if (udev_monitor_filter_add_match_subsystem_devtype(w[i].mon,
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
    } // end dev type loop

    s = pthread_attr_init(&attr);
    if (s) { fprintf(stderr, "error initializing thread attributes\n"); }
    s = pthread_create(&watch_tid, &attr, watch_loop, NULL);
    if (s) { fprintf(stderr, "error creating thread\n"); }
    pthread_attr_destroy(&attr);
}

void dev_monitor_deinit(void) {
    pthread_cancel(watch_tid);
    for (int i = 0; i < DEV_TYPE_COUNT; i++) {
        free(w[i].mon);
    }
}

int dev_monitor_scan(void) {
    struct udev *udev;
    struct udev_device *dev;

    udev = udev_new();
    if (udev == NULL) {
        fprintf(stderr, "device_monitor_scan(): failed to create udev\n");
        return 1;
    }

    for(int i = 0; i < DEV_TYPE_COUNT; i++) {
        struct udev_enumerate *ue;
        struct udev_list_entry *devices, *dev_list_entry;

        ue = udev_enumerate_new(udev);
        udev_enumerate_add_match_subsystem(ue, w[i].sub_name);
        udev_enumerate_scan_devices(ue);

        devices = udev_enumerate_get_list_entry(ue);

        udev_list_entry_foreach(dev_list_entry, devices) {
            const char *path;

            path = udev_list_entry_get_name(dev_list_entry);
            dev = udev_device_new_from_syspath(udev, path);

            if (dev != NULL) {
                if (udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
                    handle_device(dev);
                }
                udev_device_unref(dev);
            }
        }

        udev_enumerate_unref(ue);
    }
    return 0;
}

//-------------------------------
//--- static function definitions

void* watch_loop(void *p) {
    (void) p;
    struct udev_device *dev;

    while (1) {
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
        for (int i = 0; i < DEV_TYPE_COUNT; i++) {
            if (pfds[i].revents & POLLIN) {
                dev = udev_monitor_receive_device(w[i].mon);
                if (dev) {
                    handle_device(dev);
                    udev_device_unref(dev);
                } else {
                    fprintf(stderr, "no device data from receive_device(). this is an error!\n");
                }
            }
        }
    }
}

void handle_device(struct udev_device *dev) {
    const char *action = udev_device_get_action(dev);
    const char *node = udev_device_get_devnode(dev);
    const char *subsys = udev_device_get_subsystem(dev);

    if (action == NULL) {
        // scan
        if (node != NULL) {
            device_t t = check_dev_type(dev);

            if (t >= 0 && t < DEV_TYPE_COUNT) {
                dev_list_add(t, node, get_device_name(dev));
            }
        }
    } else {
        // monitor
        if (strcmp(subsys, "sound") == 0) {
            // try to act according to
            // https://github.com/systemd/systemd/blob/master/rules/78-sound-card.rules
            if (strcmp(action, "change") == 0) {
                const char* alsa_node = get_alsa_midi_node(dev);

                if (alsa_node != NULL) {
                    dev_list_add(DEV_TYPE_MIDI, alsa_node, get_device_name(dev));
                }
            } else if (strcmp(action, "remove") == 0) {
                if (node != NULL) {
                    dev_list_remove(DEV_TYPE_MIDI, node);
                }
            }
        } else {
            device_t t = check_dev_type(dev);

            if (t >= 0 && t < DEV_TYPE_COUNT) {
                if (strcmp(action, "add") == 0) {
                    dev_list_add(t, node, get_device_name(dev));
                } else if (strcmp(action, "remove") == 0) {
                    dev_list_remove(t, node);
                }
            }
        }
    }
}

device_t check_dev_type(struct udev_device *dev) {
    device_t t = DEV_TYPE_INVALID;
    const char *node = udev_device_get_devnode(dev);

    if (node) {
        // for now, just get USB devices.
        // eventually we might want to use this same system for GPIO, &c...
        if (udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
            for (int i = 0; i < DEV_TYPE_COUNT; i++) {
                if (fnmatch(w[i].node_pattern, node, 0) == 0) {
                    t = i;
                    break;
                }
            }
        }
    }
    return t;
}

// try to get midi device node from udev_device
const char* get_alsa_midi_node(struct udev_device *dev) {
    const char *subsys;
    const char *syspath;
    DIR *sysdir;
    struct dirent *sysdir_ent;
    int alsa_card, alsa_dev;
    char *result = NULL;

    subsys = udev_device_get_subsystem(dev);

    if (strcmp(subsys, "sound") == 0) {
        syspath = udev_device_get_syspath(dev);
        sysdir = opendir(syspath);

        while ((sysdir_ent = readdir(sysdir)) != NULL) {
            if (sscanf(sysdir_ent->d_name, "midiC%uD%u", &alsa_card, &alsa_dev) == 2) {
                if (asprintf(&result, "/dev/snd/%s", sysdir_ent->d_name) < 0) {
                    fprintf(stderr, "failed to create alsa device path for %s\n", sysdir_ent->d_name);
                    return NULL;
                }
            }
        }
    }

    return result;
}

// try to get product name from udev_device or its parents
const char* get_device_name(struct udev_device *dev) {
    char *current_name = NULL;
    struct udev_device *current_dev = dev;

    while (current_name == NULL) {
        current_name = (char *) udev_device_get_sysattr_value(current_dev, "product");
        current_dev = udev_device_get_parent(current_dev);

        if (current_dev == NULL) {
            break;
        }
    }

    return strdup(current_name);
}
