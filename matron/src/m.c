#include <stdio.h>
#include <monome.h>

#include "args.h"
#include "events.h"
#include "m.h"

// grid device pointer
monome_t *m = NULL;
// led state
unsigned int m_leds[16][16] = { [0 ... 15][0 ... 15] = 0 };

// grid event handlers
void m_handle_press(const monome_event_t *e, void* p) {
  event_post_monome_grid(EVENT_GRID_PRESS, e->grid.x, e->grid.y);
}

void m_handle_lift(const monome_event_t *e, void* p) {
  event_post_monome_grid(EVENT_GRID_PRESS, e->grid.x, e->grid.y);
}

void m_init() {
  const char* dev = args_monome_path();
  printf("starting libmonome\n");
  m = monome_open(dev);
  if( m == NULL) { 
	printf("m_init(): couldn't open monome device (%s)\n", dev);
  } else {
	printf("using monome device at %s:\n\n", dev);
  }
  
  if (m != NULL) { 
	monome_register_handler(m, MONOME_BUTTON_DOWN, m_handle_press, NULL);
	monome_register_handler(m, MONOME_BUTTON_UP, m_handle_lift, NULL);
	// TODO: arc handlers
	// TODO: connection handlers??
  }
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
