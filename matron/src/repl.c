#include <stdint.h>
#include <stdio.h>
#include "repl.h"

#define REPL_RX_BUF_SIZE (1024 * 128)
#define REPL_TX_BUF_SIZE (1024 * 128)

static char rx_buf[REPL_RX_BUF_SIZE];
static char tx_buf[REPL_RX_BUF_SIZE];

static unsigned int rx_idx;

void repl_process_sdl_key(SDL_KeyboardEvent *key) {
  // rx_buf[idx++] = ...
  printf("keycode: %04x \r\n", key->keysym.sym);
}

#undef REPL_RX_BUF_SIZE
#undef REPL_TX_BUF_SIZE
