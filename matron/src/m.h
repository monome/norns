
#pragma once
#include <stdint.h>

/*
  m.h - monome device glue.
  ("monome.h" is taken)
 */

// device manipulation
struct m_dev;
// set a single led
extern void m_dev_set_led(struct m_dev *md, uint8_t x, uint8_t y, uint8_t val);
// set all data for a quad
extern void m_dev_set_quad(struct m_dev *md, uint8_t quad, uint8_t *data);
// transmit data for all dirty quads
extern void m_dev_refresh(struct m_dev *md);

// get id from device pointer
extern int m_dev_id(struct m_dev* md);
// get serial string
extern const char* m_dev_serial(struct m_dev* md);
// get friendly name string from device pointer
extern const char* m_dev_name(struct m_dev* md);

// device management 
extern void m_init();
extern void m_deinit();

extern void m_add_device(const char* path);
extern void m_remove_device(const char* path);
