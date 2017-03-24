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
};

enum {
  IO_MATRON_RX,
  IO_MATRON_TX,
  IO_CRONE_RX,
  IO_COUNT
};

//--------------------------
//---- function declarations

// loop to receive data from matron
void* matron_rx_loop(void* psock);
// loop to send data to matron
void* matron_tx_loop(void* x);
// loop to receive data from crone
void* crone_rx_loop(void* x);

//----------------
//---- variables

struct sock_io sock_io[IO_COUNT];

char* url_default[IO_COUNT] = {
  "ipc:///tmp/matron_out.ipc", 
  "ipc:///tmp/matron_in.ipc",
  "ipc:///tmp/crone_out.ipc"
};

void* (*loop_func[IO_COUNT])(void*) = {
  &matron_rx_loop,
  &matron_tx_loop,
  &crone_rx_loop,
};

//-----------
//--- function defintions

void sock_io_init(struct sock_io *io, char* url, void* (*loop)(void*)) {
  // connect socket
  io->sock = nn_socket(AF_SP, NN_BUS);
  if(nn_connect(io->sock, url) < 0) {
	perror("error connecting socket");
  }
  // launch thread
  pthread_attr_t attr;
  int s;
  s = pthread_attr_init(&attr);
  if(s) { printf("error initializing thread attributes \n"); }
  s = pthread_create(&(io->tid), &attr, loop, io);
  if(s) { printf("error creating thread\n"); }
  pthread_attr_destroy(&attr);  
}

int io_init(int argc, char** argv) {
  for(int i=0; i<IO_COUNT; i++) {
	char* url;
	if(argc > (i+1)) {
	  url = argv[i+1];
	} else {
	  url = url_default[i];
	}
	sock_io_init(&(sock_io[i]), url, loop_func[i]);
  }
  return 0;
}

int io_deinit(void) {
  for(int i=0; i<IO_COUNT; i++) {
	pthread_cancel(sock_io[i].tid);
  }
  return 0;
}

void io_send_code(char* buf) {
  // FIXME this reallocate + copy is pretty dumb..
  size_t sz = strlen(buf) + 2;
  char* bufcat = calloc(sz, sizeof(char));
  snprintf(bufcat, sz, "%s\n", buf);
  struct sock_io *io = &(sock_io[IO_MATRON_TX]);
  // printf("sending to socket %d: %s (%dB)\n", io->sock, bufcat, sz);
  unsigned int tx = nn_send(io->sock, bufcat, sz, 0);
  assert(tx == sz);
  free(bufcat);
}

int io_loop(void) {
  // wait for IO threads to exit
  for(int i=0; i<IO_COUNT; i++) {
	pthread_join(sock_io[i].tid, NULL);
  }
  printf("\n\nfare well\n\n");
  return 0;
}

void* matron_rx_loop(void* p) {
  struct sock_io *io = (struct sock_io*)p;
  char msg[BUF_SIZE];
  while(1) {
	int nb = nn_recv (io->sock, msg, BUF_SIZE, 0);
	if(nb >=0) {
	  msg[nb] = '\0';
	  ui_matron_line(msg);
	}
  }
  return NULL;
}

void* matron_tx_loop(void* x) {
  (void)x;
  ui_loop(); // <-- quit when this exits
  io_deinit();
  return NULL;
}


void* crone_rx_loop(void* x) {
  (void)x;
  // TODO 
  while(1) { usleep(1000000); }
  return NULL;
}
