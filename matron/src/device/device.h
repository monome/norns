
#pragma once

#include <stdint.h>
#include <pthread.h>
#include "device_monome.h"
#include "device_hid.h"
#include "device_midi.h"
#include "device_crow.h"
#include "device_common.h"

// common data structure for all devices
union dev {
    struct dev_common base;
    struct dev_monome monome;
    struct dev_hid hid;
    struct dev_midi midi;
    struct dev_crow crow;
};

// initialize device registry
extern void devices_init(void);
// create a device from a file path
extern union dev *dev_new(
    device_t type,
    const char *path,
    const char *name,
    bool multiport_device,
    unsigned int midi_port_index
);
// destroy given device
extern void dev_delete(union dev *d);

// get id from device pointer
extern int dev_id(union dev *d);
// get serial string
extern const char *dev_serial(union dev *d);
// get friendly name string from device pointer
extern const char *dev_name(union dev *d);
