#include <stdio.h>
#include <monome.h>
#include "m.h"

char dev[128] = "/dev/ttyUSB0";

// grid device pointer
monome_t *m = NULL;
// led state
unsigned int m_leds[16][16] = { [0 ... 15][0 ... 15] = 0 };

void m_init() {
  printf("starting libmonome\n");
  // FIXME: override defaults
  m = monome_open(dev);
  if( m == NULL) { 
	printf("m_init(): couldn't open monome device (%s)\n", dev);
  } else {
	printf("using monome device at %s:\n\n", dev);
  }
  // FIXME:
  // how to set up connection events?
}

// set hardware
void m_grid_set_led(int x, int y,  int val) {
  m_leds[x][y] = val;
  if(m != NULL) { 
	monome_led_set(m, x, y, m_leds[x][y]);
  }
}

void m_arc_set_led(int id, int rho,  int val) {
  //..
}

// set handlers
void m_set_grid_press(monome_event_callback_t cb, void* data) {
  printf("m_set_grid_press\n");
  if (m != NULL) { 
	monome_register_handler(m, MONOME_BUTTON_DOWN, cb, data);
  }
}
void m_set_grid_lift(monome_event_callback_t cb, void* data) {
  printf("m_set_grid_press\n");
  if (m != NULL) { 
	monome_register_handler(m, MONOME_BUTTON_UP, cb, data);
  }
}
void m_set_arc_turn();
