
#pragma once

#include <stdint.h>
#include <pthread.h>
#include "device_monome.h"
#include "device_input.h"
#include "device_common.h"


#define TEST_NULL_AND_FREE(p) if((p)!=NULL) { free(p); }


// common data structure for all devices
union dev {
  struct dev_common base;
  struct dev_monome monome;
  struct dev_input input;
};

// initialize device registry
extern void devices_init(void);
// create a device from a file path
extern union dev* dev_new(device_t type, const char* path);
// destroy given device
extern int dev_delete(union dev* d);

// get id from device pointer
extern int dev_id(union dev* d);
// get serial string
extern const char* dev_serial(union dev* d);
// get friendly name string from device pointer
extern const char* dev_name(union dev* d);
