
#pragma once

/*
  m.h - monome device glue.
  ("monome.h" is taken)
 */

extern void m_init();
extern void m_deinit();

// set hardware
extern void m_grid_set_led(int x, int y,  int val);
extern void m_arc_set_led(int id, int rho,  int val);

// TODO: col/row/quad/intense/&c ?
