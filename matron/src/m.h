
#pragma once

/*
  m.h - monome device glue.
  ("monome.h" is taken)
 */

extern void m_init();

// set hardware
extern void m_grid_set_led(int x, int y,  int val);
extern void m_arc_set_led(int id, int rho,  int val);

// set handlers
extern void m_set_grid_press();
extern void m_set_grid_lift();
extern void m_set_arc_turn();
