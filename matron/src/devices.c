/*
  usb_monitor.c

  

 */

#include <errno.h>
#include <libudev.h>
#include <locale.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "events.h"
#include "m.h"

//---------------------
//--- types and defines

#define SUBNAME_SIZE 128
#define NUM_WATCH 2
#define WATCH_TIMEOUT_MS 100

enum {
  // libmonome devices
  DEVICE_TYPE_MONOME,
  // usb hid mouse
  DEVICE_TYPE_MOUSE,
  // usb hid keyboard
  DEVICE_TYPE_KEYBOARD,
  // usb hid joystick
  DEVICE_TYPE_JS,
  // MIDI
  DEVICE_TYPE_MIDI,
  // generic usb serial
  DEVICE_TYPE_USB_SERIAL_GENERIC,
  // generic HID
  DEVICE_TYPE_HID_GENERIC
};

//-------------------------
//----- static variables

// subsystem names to watch
const char* subname[NUM_WATCH] = {
  "input",
  "tty"
};

// file descriptors to watch/poll
struct pollfd pfds[NUM_WATCH];
// udev monitor objects
struct udev_monitor *mon[NUM_WATCH];
// thread for polling all the watched file descriptors
pthread_t watch_tid;

//--------------------------------
//--- static function declarations
static void* watch_loop(void* data);

static bool check_monome_device(struct udev_device *dev, node) {
  /* if(strcmp(udev_device_get_subsystem(dev), "tty") == 0) { */
  /* 	printf("device is tty...\n"); */
  /* 	if (udev_device_get_parent_with_subsystem_devtype(dev, "usb-serial", NULL)) { */
  /* 	  printf("device is usb-serial... \n"); */
  /* 	  return 1; */
  /* 	} */
  /* } */
  // udev doesn't find the usb-serial parent on disconnect... 
  return 0;
}

static bool check_hid_device(struct udev_device *dev, node) {
	if (udev_device_get_parent_with_subsystem_devtype(dev, "hid", NULL)) {
	  return 1;
	}
  return 0;
}


//--------------------------------
//---- extern function definitions

void devices_init(void) {
  struct udev *udev;
  for(int i=0; i<NUM_WATCH; i++) {
	mon[i] = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(mon[i], subname[i], NULL);
	udev_monitor_enable_receiving(mon[i]);
	pfds[i].fd = udev_monitor_get_fd(mon[i]);
	pfds[i].events = POLLIN;
  }
  // create monitoring thread
  pthread_attr_t attr;
  int s;
  s = pthread_attr_init(&attr);
  if(s) { printf("error initializing thread attributes \n"); }
  s = pthread_create(&watch_tid, &attr, watch_loop, NULL);
  if(s) { printf("error creating thread\n"); }
  pthread_attr_destroy(&attr);
}

void devices_deinit(void) {
  pthread_cancel(watch_tid);
  for(int i=0; i<NUM_WATCH; i++) {
	free(mon[i]);
  }
}

//-------------------------------
//--- static function definitions

void* watch_loop(void* x) {
  struct udev_device *dev;
  struct timeval tv;
  int fd;
  int ret;
  const char* node;
  
  while(1) {

	if (poll(pfds, NUM_WATCH, WATCH_TIMEOUT_MS) < 0)
	  switch (errno) {
	  case EINVAL:
		perror("error in poll()");
		exit(1);
	  case EINTR:
	  case EAGAIN:
		continue;
	  }

	// see which monitor has data
	for(int i=0; i<NUM_WATCH; i++) {
	  if(pfds[i].revents & POLLIN) {
		dev = udev_monitor_receive_device(mon[i]);
		if (dev) {
		  /*
		  printf("device:\n");
		  printf(" node: %s\n", udev_device_get_devnode(dev));
		  printf(" subsystem: %s\n", udev_device_get_subsystem(dev));
		  printf(" action: %s\n", udev_device_get_action(dev));
		  */
		  node = udev_device_get_devnode(dev);
		  if(node) { 
			switch(udev_device_get_action(dev)[0]) {
			case 'a':
			  if(check_monome_device(dev)) { 
				printf("adding monome device: %s\n", node);
				m_add_device(udev_device_get_devnode(dev));
			  }
			  if(check_hid_device(dev)) {
				printf("(TODO) adding hid device: %s\n", node);
			  }
			  break;
			case 'r':
			  if(check_monome_device(dev)) {
				printf("removing monome device: %s\n", node);
				m_remove_device(udev_device_get_devnode(dev));
			  }
			  if(check_hid_device(dev)) {
				printf("(TODO) removing hid device: %s\n", node);
			  }
			  break;
			default:
			  ;;
			}
		  }
		  udev_device_unref(dev);
		  
		  fflush(stdout);
		}
		else {
		  printf("no device data from receive_device(). this is an error!\n");
		}
	  }
	}
  }
	
}
