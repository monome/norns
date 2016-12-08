#include <stdlib.h>
#include <stdio.h>
#include <monome.h>

#include "lua.h"

//----- led state

unsigned int leds[16][16] = { [0 ... 15][0 ... 15] = 0 };

void set_led(monome_t *m, int x, int y, int val) {
  leds[x][y] = val;
  monome_led_set(m, x, y, leds[x][y]);
}

//----- handlers

void handle_press(const monome_event_t *e, void *data) {
  set_led(e->monome, e->grid.x, e->grid.y, 1);
}
 
void handle_lift(const monome_event_t *e, void *data) {
  set_led(e->monome, e->grid.x, e->grid.y, 0);
}

int main(const char argc, const char** argv) {
  const char device[] = "/dev/ttyUSB0";
  monome_t *monome;

  if( !(monome = monome_open(device, "8000")) ) {
	printf("error opening device %s , exiting\r\n", device);
	return -1;
  }

  monome_led_all(monome, 0);
  monome_register_handler(monome, MONOME_BUTTON_DOWN, handle_press, NULL);
  monome_register_handler(monome, MONOME_BUTTON_UP, handle_lift, NULL);

  monome_event_loop(monome);
  monome_close(monome);
}
