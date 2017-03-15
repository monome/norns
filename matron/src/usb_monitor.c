/*
  usb_monitor.c
*/

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

#include "events.h"
#include "m.h"

//---------------------
//--- types and defines

#define SUB_NAME_SIZE 32
#define NODE_NAME_SIZE 128
#define WATCH_TIMEOUT_MS 100


#define NUM_DEV_TYPES 2

typedef enum {
  // libmonome devices
  USB_DEV_MONOME = 0,
  // hid devices
  USB_DEV_HID = 1
} usb_dev_t;

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
// FIXME: these names and patterns should be taken from config.lua
static struct watch w[NUM_DEV_TYPES] = {
  {
	.sub_name = "tty",
	.node_pattern = "/dev/ttyUSB.*"
  },
  {
	.sub_name = "input",
	.node_pattern = "/dev/input/event.*"
  }
};

// file descriptors to watch/poll
struct pollfd pfds[NUM_DEV_TYPES];
// thread for polling all the watched file descriptors
pthread_t watch_tid;

//--------------------------------
//--- static function declarations
static void* watch_loop(void* data);
static void handle_device(struct udev_device *dev);
static void add_device(struct udev_device* dev, usb_dev_t t);
static void remove_device(struct udev_device* dev, usb_dev_t t);
static usb_dev_t check_dev_type (struct udev_device *dev);

//--------------------------------
//---- extern function definitions

void usb_monitor_init(void) {
  struct udev *udev;
  pthread_attr_t attr;
  int s;
  
  for(int i=0; i<NUM_DEV_TYPES; i++) {
	w[i].mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(w[i].mon, w[i].sub_name, NULL);
	udev_monitor_enable_receiving(w[i].mon);

	pfds[i].fd = udev_monitor_get_fd(w[i].mon);
	pfds[i].events = POLLIN;
	
	if(regcomp(&w[i].node_regex, w[i].node_pattern, 0)) {
	  printf("error compiling regex for device pattern: %s\n", w[i].node_pattern);
	}	
  }
  s = pthread_attr_init(&attr);
  if(s) { printf("error initializing thread attributes \n"); }
  s = pthread_create(&watch_tid, &attr, watch_loop, NULL);
  if(s) { printf("error creating thread\n"); }
  pthread_attr_destroy(&attr);
}

void usb_monitor_deinit(void) {
  pthread_cancel(watch_tid);
  for(int i=0; i<NUM_DEV_TYPES; i++) {
	free(w[i].mon);
  }
}


int usb_monitor_scan(void) {
  struct udev *udev;
  struct udev_device *dev;
  const char* node;
  
  udev = udev_new();
  if (!udev) {
  printf("usb_monitor_scan(): failed to create udev\n"); fflush(stdout);
	return 1;
  }
  
  for(int i=0; i<NUM_DEV_TYPES; i++) {
	struct udev_enumerate *ue;
	struct udev_list_entry *devices, *dev_list_entry;
	printf("scanning for devices of type %s\n", w[i].sub_name); fflush(stdout);
	ue = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(ue, w[i].sub_name);
	udev_enumerate_scan_devices(ue);
	devices = udev_enumerate_get_list_entry(ue);
	udev_list_entry_foreach(dev_list_entry, devices) {
	  const char *path;
	  path = udev_list_entry_get_name(dev_list_entry);
	  dev = udev_device_new_from_syspath(udev, path);
	  // FIXME: gotta be a way to get only usb-serial and HID... 
	  //	  dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL);
	  if (dev !=NULL) {
		node = udev_device_get_devnode(dev);
		if(node != NULL) {
		  add_device(dev, check_dev_type(dev));
		}
		udev_device_unref(dev);
	  }
	  // printf("continue device enumeration\n"); fflush(stdout);
	}	
	udev_enumerate_unref(ue);
  }

  return 0;
}

//-------------------------------
//--- static function definitions

void* watch_loop(void* x) {
  struct udev_device *dev;
  struct timeval tv;
  int fd;
  int ret;
  
  while(1) {
	if (poll(pfds, NUM_DEV_TYPES, WATCH_TIMEOUT_MS) < 0)
	  switch (errno) {
	  case EINVAL:
		perror("error in poll()");
		exit(1);
	  case EINTR:
	  case EAGAIN:
		continue;
	  }

	// see which monitor has data
	for(int i=0; i<NUM_DEV_TYPES; i++) {
	  if(pfds[i].revents & POLLIN) {
		dev = udev_monitor_receive_device(w[i].mon);
		if (dev) {		  
		  handle_device(dev);
		  udev_device_unref(dev);
		}
		else {
		  printf("no device data from receive_device(). this is an error!\n");
		}
	  }
	}
  }	
}

void add_device(struct udev_device* dev, usb_dev_t t) {
  const char* node = udev_device_get_devnode(dev);
  switch(t) {
  case USB_DEV_MONOME:
	printf("\n-----\nadding monome device: %s\n", node); fflush(stdout);
	m_add_device(node); 
	break;
  case USB_DEV_HID:
	printf("(TODO) adding hid device: %s\n", node); fflush(stdout);
	  // TODO
	break;
  default:
	;; // do nothing
  }
}

void remove_device(struct udev_device* dev, usb_dev_t t) {
  const char* node = udev_device_get_devnode(dev);
  switch(t) {
  case USB_DEV_MONOME:
	printf("\n-----\nremoving monome device: %s\n", node); fflush(stdout);
	m_remove_device(node); 
	break;
  case USB_DEV_HID:
	printf("(TODO) removing hid device: %s\n", node); fflush(stdout);
	// TODO
	break;
  }
}

void handle_device(struct udev_device *dev) {
  usb_dev_t t = check_dev_type(dev);
  const char* act = udev_device_get_action(dev);
  if(act[0] == 'a') {
	add_device(dev, t);
  } else if (act[0] == 'r') {
	remove_device(dev, t);
  }
}

usb_dev_t check_dev_type (struct udev_device *dev) {
  static char msgbuf[128];
  usb_dev_t t = -1;
  const char* node = udev_device_get_devnode(dev);
  int reti;
  if(node) {
	for(int i=0; i < NUM_DEV_TYPES; i++) {
	  fflush(stdout);
	  reti = regexec(&w[i].node_regex, node, 0, NULL, 0);
	  if(reti == 0) { 
		t = i;
		break;		
	  }
	  else if (reti == REG_NOMATCH) {
		;; // nothing to do
	  }
	  else {
		regerror(reti, &w[i].node_regex, msgbuf, sizeof(msgbuf));
		fprintf(stderr, "regex match failed: %s\n", msgbuf);
		exit(1);
	  }	  
	}
  }
  return t;
}
