#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nng/nng.h>
#include <nng/protocol/bus0/bus.h>
#include <nng/transport/ws/websocket.h>

#include "io.h"
#include "ui.h"

//-----------------
//---- defines, types

#define BUF_SIZE 4096

struct sock_io {
  nng_socket sock;
  nng_dialer dialer;
  pthread_t tid;
  bool has_thread; // ugh
};

//--------------------------
//---- function declarations

// loop to receive data from matron
void *matron_rx_loop(void *psock);
// loop to send data to matron
void *tx_loop(void *x);
// loop to receive data from crone
void *crone_rx_loop(void *x);

//----------------
//---- variables

struct sock_io sock_io[IO_COUNT];

char *url_default[IO_COUNT] = {
  "ws://127.0.0.1:5555/",
  "ws://127.0.0.1:5556/",
  NULL,
};

void * (*loop_func[IO_COUNT])(void *) = {
  &matron_rx_loop,
  &crone_rx_loop,
  &tx_loop
};

//-----------
//--- function defintions

void sock_io_init(struct sock_io *io, char *url, void * (*loop)(void *)) {
  int rv;

  // connect socket
  if (url) {
    if ( ((rv = nng_bus0_open(&(io->sock))) != 0) ||
         ((rv = nng_dialer_create(&(io->dialer), io->sock, url)) != 0) ||
         ((rv = nng_dialer_set_bool(io->dialer, NNG_OPT_WS_SEND_TEXT, true)) != 0) ||
         ((rv = nng_dialer_set_bool(io->dialer, NNG_OPT_WS_RECV_TEXT, true)) != 0) ||
         ((rv = nng_dialer_start(io->dialer, 0)) != 0) ) {
      ui_matron_line("opening connection failed: (");
      ui_matron_line(url);
      ui_matron_line(") ");
      ui_matron_line(nng_strerror(rv));
      ui_matron_line("\n");
    }
  }

  // launch thread if loop function defined
  io->has_thread = false;
  if (loop == NULL) { return; }
  pthread_attr_t attr;
  int s;
  io->has_thread = true;
  s = pthread_attr_init(&attr);
  if (s) { printf("error initializing thread attributes \n"); }
  s = pthread_create(&(io->tid), &attr, loop, io);
  if (s) { printf("error creating thread\n"); }
  pthread_attr_destroy(&attr);
}

int io_init(int argc, char **argv) {
  for(int i = 0; i < IO_COUNT; i++) {
    char *url;
    if( argc > (i + 1) ) {
      url = argv[i + 1];
    } else {
      url = url_default[i];
    }
    sock_io_init(&(sock_io[i]), url, loop_func[i]);
  }
  return 0;
}

int io_deinit(void) {
  for(int i = 0; i < IO_COUNT; i++) {
    if(sock_io[i].has_thread) {
      pthread_cancel(sock_io[i].tid);
    }
  }
  return 0;
}

void io_send_line(int sockid, char *buf) {
  // FIXME this reallocate + copy is pretty dumb..
  int rv;
  size_t sz = strlen(buf) + 2;
  char *bufcat = calloc( sz, sizeof(char) );
  snprintf(bufcat, sz, "%s\n", buf);
  struct sock_io *io = &(sock_io[sockid]);
  if ((rv = nng_send(io->sock, bufcat, sz, 0)) != 0) {
    // TODO: warn on send failure?
  }
  free(bufcat);
}

int io_loop(void) {
  // wait for IO threads to exit
  for(int i = 0; i < IO_COUNT; i++) {
    if(sock_io[i].has_thread) {
      pthread_join(sock_io[i].tid, NULL);
    }
  }
  printf("\nfare well\n\n");
  return 0;
}

void *rx_loop(void *p, void (*ui_func)(const char *)) {
  struct sock_io *io = (struct sock_io *)p;
  char msg[BUF_SIZE];
  size_t nb;
  int rv;
  while (1) {
    nb = BUF_SIZE;
    if ((rv = nng_recv(io->sock, msg, &nb, 0)) == 0) {
      if (nb > 0) {
        msg[nb] = '\0';
        ui_func(msg);
      }
    }
  }
  return NULL;
}

void *matron_rx_loop(void *p) {
  return rx_loop(p, &ui_matron_line);
}

void *crone_rx_loop(void *p) {
  return rx_loop(p, &ui_crone_line);
}

void *tx_loop(void *x) {
  (void)x;
  ui_loop(); // <-- quit when this exits
  io_deinit();
  return NULL;
}
