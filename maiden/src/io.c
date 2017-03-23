#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include "io.h"
#include "ui.h"

//-----------------
//---- defines

#define BUF_SIZE 4096;

//----------------
//---- variables

// client thread id's
pthread_t matron_tx_tid, matron_rx_tid;
pthread_t crone_rx_tid;

//--------------------------
//---- function declarations

// run matron process and redirect its IO
int run_matron(void);
// loop to receive data from matron
void* matron_rx_loop(void* psock);
// loop to send data to matron
void* matron_tx_loop(void* x);

// loop to receive data from crone
void* crone_rx_loop(void* x);

// utility to launch a joinable thread
void launch_thread(pthread_t *tid, void *(*start_routine) (void *), void* data);


int io_init(void) {
  
  // create threads to handle the children's i/o
  launch_thread(&matron_tx_tid, &matron_tx_loop, NULL);
  launch_thread(&matron_rx_tid, &matron_rx_loop, NULL);
  launch_thread(&crone_rx_tid, &crone_rx_loop, NULL);
  return 0;
  
}

int io_deinit(void) {
  pthread_cancel(matron_rx_tid);
  pthread_cancel(matron_tx_tid);
}

void io_send_code(char* buf, size_t len) {  

}

int io_loop(void) {
  // wait for threads to exit
  pthread_join(matron_tx_tid, NULL);
  pthread_join(matron_rx_tid, NULL);
  pthread_join(crone_rx_tid, NULL);
  printf("\n\nfare well\n\n");
  return 0;
}


void* matron_rx_loop(void* psock) {
}

void* matron_tx_loop(void* x) {
  ui_loop();
  io_deinit();
}


void* crone_rx_loop(void* x) {
  // TODO
  while(1) { usleep(1000000); }
}

void launch_thread(pthread_t *tid, void *(*start_routine) (void *), void* data) {
  pthread_attr_t attr;
  int s;
  s = pthread_attr_init(&attr);
  if(s) { printf("error initializing thread attributes \n"); }
  s = pthread_create(tid, &attr, start_routine, data);
  if(s) { printf("error creating thread\n"); }
  pthread_attr_destroy(&attr);
}
