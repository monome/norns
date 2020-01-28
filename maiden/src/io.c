#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include "io.h"
#include "ui.h"

//-----------------
//---- defines, types

#define BUF_SIZE 4096

struct sock_io {
  int sock;
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
  "ws://localhost:5555/",
  "ws://localhost:5556/",
  NULL,
};

void * (*loop_func[IO_COUNT])(void *) = {
  &matron_rx_loop,
  &crone_rx_loop,
  &tx_loop
};

//-----------
//--- function defintions

void sock_io_init( struct sock_io *io, char *url, void * (*loop)(void *) ) {
  // connect socket
  if(url) {
    io->sock = nn_socket(AF_SP, NN_BUS);
    if(nn_connect(io->sock, url) < 0) {
      perror("error connecting socket");
    }
  }
  // launch thread if loop function defined
  io->has_thread = false;
  if(loop == NULL) { return; }
  pthread_attr_t attr;
  int s;
  io->has_thread = true;
  s = pthread_attr_init(&attr);
  if(s) { printf("error initializing thread attributes \n"); }
  s = pthread_create(&(io->tid), &attr, loop, io);
  if(s) { printf("error creating thread\n"); }
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
  size_t sz = strlen(buf) + 2;
  char *bufcat = calloc( sz, sizeof(char) );
  snprintf(bufcat, sz, "%s\n", buf);
  struct sock_io *io = &(sock_io[sockid]);
  unsigned int tx = nn_send(io->sock, bufcat, sz, 0);
  assert(tx == sz);
  free(bufcat);
}

int io_loop(void) {
  // wait for IO threads to exit
  for(int i = 0; i < IO_COUNT; i++) {
    if(sock_io[i].has_thread) {
      pthread_join(sock_io[i].tid, NULL);
    }
  }
  printf("\n\nfare well\n\n");
  return 0;
}

void *matron_rx_loop(void *p) {
  struct sock_io *io = (struct sock_io *)p;
  char msg[BUF_SIZE];
  while(1) {
    int nb = nn_recv (io->sock, msg, BUF_SIZE, 0);
    if(nb >= 0) {
      msg[nb] = '\0';
      ui_matron_line(msg);
    }
  }
  return NULL;
}

void *crone_rx_loop(void *p) {
  struct sock_io *io = (struct sock_io *)p;
  char msg[BUF_SIZE];
  while(1) {
    int nb = nn_recv (io->sock, msg, BUF_SIZE, 0);
    if(nb >= 0) {
      msg[nb] = '\0';
      ui_crone_line(msg);
    }
  }
  return NULL;
}

void *tx_loop(void *x) {
  (void)x;
  ui_loop(); // <-- quit when this exits
  io_deinit();
  return NULL;
}
