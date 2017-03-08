#include <stdio.h>
#include <monome.h>
#include <pthread.h>

#include "args.h"
#include "events.h"
#include "m.h"

// grid device pointer
monome_t *m = NULL;
// led state
unsigned int m_leds[16][16] = { [0 ... 15][0 ... 15] = 0 };

// thread id
pthread_t tid;

static void* m_run(void* p) {
  printf("running the monome event loop \n"); fflush(stdout);
  monome_event_loop(m);
}

// grid event handlers
void m_handle_press(const monome_event_t *e, void* p) {
  printf("m_handle_press(): posting event\n"); fflush(stdout);
  event_post_monome_grid(EVENT_GRID_PRESS, e->grid.x, e->grid.y);
}

void m_handle_lift(const monome_event_t *e, void* p) {
  printf("m_handle_lift(): posting event\n"); fflush(stdout);
  event_post_monome_grid(EVENT_GRID_LIFT, e->grid.x, e->grid.y);
}

void m_init() {
  const char* dev = args_monome_path();
  printf("starting libmonome\n"); fflush(stdout);
  m = monome_open(dev);
  if( m == NULL) { 
	printf("m_init(): couldn't open monome device (%s)\n", dev); fflush(stdout);
  } else {
	printf("using monome device at %s:\n\n", dev); fflush(stdout);
  }
  
  if (m != NULL) {
	pthread_attr_t attr;
	int s;

	monome_register_handler(m, MONOME_BUTTON_DOWN, m_handle_press, NULL);
	monome_register_handler(m, MONOME_BUTTON_UP, m_handle_lift, NULL);

	s = pthread_attr_init(&attr);
	if(s) {
	  printf("m_init(): error initializing thread attributes \n");
	  fflush(stdout);
	}
	s = pthread_create(&tid, &attr, &m_run, NULL);
	if(s) {
	  printf("m_init(): error creating thread\n");
	  fflush(stdout);
	}
	pthread_attr_destroy(&attr);
  }

}

// FIXME: hey, call this sometime huh?
void m_deinit() {
  pthread_cancel(tid);
}

// set hardware
void m_grid_set_led(int x, int y,  int val) {
  m_leds[x][y] = val;
  if(m != NULL) { 
	monome_led_set(m, x, y, m_leds[x][y]);
  }
}

void m_arc_set_led(int id, int rho,  int val) {
  //.. TODO
}
