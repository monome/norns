/*
 * device_monitor.c
 */

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <libudev.h>
#include <locale.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "device.h"
#include "device_hid.h"
#include "device_list.h"
#include "device_monome.h"

#include "events.h"

#define SUB_NAME_SIZE 32
#define NODE_NAME_SIZE 128
#define WATCH_TIMEOUT_MS 100

// enumerate unix files to watch
enum {
    DEV_FILE_TTY = 0,
    DEV_FILE_INPUT = 1,
    DEV_FILE_SOUND = 2,
    DEV_FILE_NONE = 3,
};
#define DEV_FILE_COUNT 3

struct udev_monitor *mon[DEV_FILE_COUNT];

static const char *dev_file_name[] = {"tty", "input", "sound", "none"};

//-------------------------
//----- static variables

// file descriptors to watch/poll
struct pollfd pfds[DEV_FILE_COUNT];
// thread for polling all the watched file descriptors
pthread_t watch_tid;

//--------------------------------
//--- static function declarations
static void rm_dev(struct udev_device *dev, int dev_file);
static void rm_dev_tty(struct udev_device *dev, const char *node);

static void add_dev(struct udev_device *dev, int dev_file);
static void add_dev_tty(struct udev_device *dev);
static void add_dev_input(struct udev_device *dev);
static void add_dev_sound(struct udev_device *dev);

static int is_dev_monome_grid(struct udev_device *dev);
static int is_dev_crow(struct udev_device *dev);

static void *watch_loop(void *data);

// try to get MIDI device name from ALSA
// returns a newly-allocated string (may be NULL)
static const char *get_alsa_midi_node(struct udev_device *dev);

// try to get product name from udev_device or its parents
// returns a newly-allocated string (may be NULL)
static char *get_device_name(struct udev_device *dev);

static inline void print_watch_error(const char *msg, int file_idx) {
    fprintf(stderr, "error: %s on subsystem %s", msg, dev_file_name[file_idx]);
}

//--------------------------------
//---- extern function definitions
void dev_monitor_init(void) {
    struct udev *udev = NULL;
    pthread_attr_t attr;
    int s;

    udev = udev_new();
    assert(udev);

    for (int fidx = 0; fidx < DEV_FILE_COUNT; ++fidx) {
        mon[fidx] = NULL;
        struct udev_monitor *m = udev_monitor_new_from_netlink(udev, "udev");
        if (m == NULL) {
            print_watch_error("couldn't create udev monitor", fidx);
            continue;
        }
        if (udev_monitor_filter_add_match_subsystem_devtype(m, dev_file_name[fidx], NULL) < 0) {
            print_watch_error("couldn't add subsys filter", fidx);
            continue;
        }
        if (udev_monitor_enable_receiving(m) < 0) {
            print_watch_error("failed to enable monitor", fidx);
            continue;
        }
        pfds[fidx].fd = udev_monitor_get_fd(m);
        pfds[fidx].events = POLLIN;
        mon[fidx] = m;
    }

    s = pthread_attr_init(&attr);
    if (s) {
        fprintf(stderr, "error initializing thread attributes\n");
    }
    s = pthread_create(&watch_tid, &attr, watch_loop, NULL);
    if (s) {
        fprintf(stderr, "error creating thread\n");
    }
    pthread_attr_destroy(&attr);
}

void dev_monitor_deinit(void) {
    pthread_cancel(watch_tid);
    for (int fidx = 0; fidx < DEV_FILE_COUNT; ++fidx) {
        if (mon[fidx] != NULL) {
            free(mon[fidx]);
        }
    }
}

int dev_monitor_scan(void) {
    struct udev *udev;
    struct udev_device *dev;

    udev = udev_new();
    if (udev == NULL) {
        fprintf(stderr, "dev_monitor: failed to create udev\n");
        return 1;
    }

    for (int fidx = 0; fidx < DEV_FILE_COUNT; ++fidx) {
        struct udev_enumerate *ue;
        struct udev_list_entry *devices, *dev_list_entry;

        ue = udev_enumerate_new(udev);
        udev_enumerate_add_match_subsystem(ue, dev_file_name[fidx]);
        udev_enumerate_scan_devices(ue);
        devices = udev_enumerate_get_list_entry(ue);

        udev_list_entry_foreach(dev_list_entry, devices) {
            const char *path;

            path = udev_list_entry_get_name(dev_list_entry);
            dev = udev_device_new_from_syspath(udev, path);

            if (dev != NULL) {
                if (udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
                    add_dev(dev, fidx);
                }
            }
            udev_device_unref(dev);
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

    while (1) {
        if (poll(pfds, DEV_FILE_COUNT, WATCH_TIMEOUT_MS) < 0) {
            switch (errno) {
            case EINVAL:
                perror("error in poll()");
                exit(1);
            case EINTR:
            case EAGAIN:
                continue;
            }
        }

        for (int fidx = 0; fidx < DEV_FILE_COUNT; ++fidx) {
            if (pfds[fidx].revents & POLLIN) {
                dev = udev_monitor_receive_device(mon[fidx]);
                if (dev) {
                    const char *action = udev_device_get_action(dev);
                    if (action != NULL) {
                        if (strcmp(action, "remove") == 0) {
                            rm_dev(dev, fidx);
                        } else {
                            add_dev(dev, fidx);
                        }
                    } else {
                        fprintf(stderr, "dev_monitor error: unknown device action\n");
                    }
                    udev_device_unref(dev);
                } else {
                    fprintf(stderr, "dev_monitor error: no device data\n");
                }
            }
        }
    }
}

void rm_dev(struct udev_device *dev, int dev_file) {
    const char *node = udev_device_get_devnode(dev);
    if (node == NULL) {
        return;
    }
    switch (dev_file) {
    case DEV_FILE_TTY:
        rm_dev_tty(dev, node);
        break;
    case DEV_FILE_INPUT:
        dev_list_remove(DEV_TYPE_HID, node);
        break;
    case DEV_FILE_SOUND:
        dev_list_remove(DEV_TYPE_MIDI, node);
        break;
    case DEV_FILE_NONE:
    default:;
        ;
    }
}

void rm_dev_tty(struct udev_device *dev, const char *node) {
    // fprintf(stderr, "rm_dev_tty: %s\n", node);

    if (fnmatch("/dev/ttyUSB*", node, 0) == 0) {
        fprintf(stderr, "dev_monitor: got ttyUSB, assuming grid\n");
        dev_list_remove(DEV_TYPE_MONOME, node);
        return;
    }

    if (is_dev_monome_grid(dev)) {
        fprintf(stderr, "dev_monitor: TTY appears to be ACM grid\n");
        dev_list_remove(DEV_TYPE_MONOME, node);
        return;
    }

    if (is_dev_crow(dev)) {         
        dev_list_remove(DEV_TYPE_CROW, node);
        return;
    }
    
    fprintf(stderr, "dev_monitor: unmatched TTY device was removed from %s\n", node);

}

void add_dev(struct udev_device *dev, int fidx) {
    switch (fidx) {
    case DEV_FILE_TTY:
        add_dev_tty(dev);
        break;
    case DEV_FILE_INPUT:
        add_dev_input(dev);
        break;
    case DEV_FILE_SOUND:
        add_dev_sound(dev);
        break;
    case DEV_FILE_NONE:
    default:
        break;
    }
}

void add_dev_tty(struct udev_device *dev) {
    const char *node = udev_device_get_devnode(dev);
    if (node == NULL) {
        // not a physical device. for the moment we are not concerned with input from virtual inputs
        return;
    }
    char *name = get_device_name(dev);
    if (fnmatch("/dev/ttyUSB*", node, 0) == 0) {
        fprintf(stderr, "dev_monitor: got ttyUSB, assuming grid\n");
        dev_list_add(DEV_TYPE_MONOME, node, name);
    } else if (is_dev_monome_grid(dev)) {
        fprintf(stderr, "dev_monitor: TTY appears to be ACM grid\n");
        dev_list_add(DEV_TYPE_MONOME, node, name);
    } else if (is_dev_crow(dev)) {
        fprintf(stderr, "tty is a crow\n");
        dev_list_add(DEV_TYPE_CROW, node, name);
    } else {
        fprintf(stderr, "dev_monitor: unmatched TTY device %s at %s\n", name, node);
    }
}

void add_dev_input(struct udev_device *dev) {
    const char *node = udev_device_get_devnode(dev);
    if (node == NULL) {
	    fprintf(stderr, "dev_monitor: skipping node-less entry in /dev/input\n");
	    return;
    }
    char *name = get_device_name(dev);
    dev_list_add(DEV_TYPE_HID, node, name);
}

void add_dev_sound(struct udev_device *dev) {
    // try to act according to
    // https://github.com/systemd/systemd/blob/master/rules/78-sound-card.rules
    const char *alsa_node = get_alsa_midi_node(dev);
    if (alsa_node != NULL) {
	char *name = get_device_name(dev);
	fprintf(stderr, "dev_monitor: adding midi device %s\n", name);
        dev_list_add(DEV_TYPE_MIDI, alsa_node, name);
    }
}

// try to get midi device node from udev_device
const char *get_alsa_midi_node(struct udev_device *dev) {
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
                    fprintf(stderr, "dev_monitor: failed to create alsa device path for %s\n", sysdir_ent->d_name);
                    return NULL;
                }
            }
        }
    }

    return result;
}

char *get_device_name(struct udev_device *dev) {
    char *current_name = NULL;
    struct udev_device *current_dev = dev;

    while (current_name == NULL) {
        current_name = (char *)udev_device_get_sysattr_value(current_dev, "product");
        current_dev = udev_device_get_parent(current_dev);

        if (current_dev == NULL) {
            break;
        }
    }

    return strdup(current_name);
}

int is_dev_monome_grid(struct udev_device *dev) {
    const char *vendor, *model, *vendor_id, *model_id;

    vendor = udev_device_get_property_value(dev, "ID_VENDOR");
    model = udev_device_get_property_value(dev, "ID_MODEL");
    vendor_id = vendor != NULL ? vendor : udev_device_get_property_value(dev, "ID_VENDOR_ID");
    model_id = model != NULL ? model : udev_device_get_property_value(dev, "ID_MODEL_ID");

    fprintf(stderr, "dev_monitor: vendor=%s model=%s\n", vendor_id, model_id);

    if (vendor == NULL || model == NULL) {
        return 0;
    }

    if (strcmp(vendor, "monome") != 0) {
        return 0;
    }

    if (strcmp(model, "grid") == 0) {
        // a monome grid
        return 1;
    }
    if (strcmp(model, "monome") == 0) {
        // probably a clone
        return 1;
    }

    return 0;
}

int is_dev_crow(struct udev_device *dev) { 
    const char *device_product_string = udev_device_get_property_value(dev, "ID_MODEL");
    if(device_product_string != NULL) {
        return strcmp(device_product_string, "crow:_telephone_line") == 0;
    }
   return 0;
}
