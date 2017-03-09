#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "io.h"
#include "ui.h"

//-----------------
//---- defines

#define PIPE_READ 0
#define PIPE_WRITE 1
#define PIPE_BUF_SIZE 1024

//----------------
//---- variables

int matron_pipe_in[2];
int matron_pipe_out[2];

int crone_pipe_out[2];

// child process id's
pid_t matron_pid;
pid_t crone_pid;

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
// run the audio process and redirect its IO
void run_crone(void);
// loop to receive data from clone
void* crone_rx_loop(void* x);
// kill the audio process
void quit_crone(void);

// utility to launch a joinable thread
void launch_thread(pthread_t *tid, void *(*start_routine) (void *), void* data);
// utility to close a pipe
void cleanup_pipe(int pipe[2]);


int io_init(void) {
  
  // create pipes for i/o redirection
  if(pipe(matron_pipe_in) < 0) {
	perror("allocating pipe for matron input redirect");
	return -1;
  }
  if(pipe(matron_pipe_out) < 0) {
	perror("allocating pipe for matron output redirect");
	cleanup_pipe(matron_pipe_in);
	return -1;
  }
  if(pipe(crone_pipe_out) < 0) {
	perror("allocating pipe for crone output redirect");
	cleanup_pipe(matron_pipe_in);
	cleanup_pipe(matron_pipe_out);
	return -1;
  }
  
  // fork a child process to run matron
  matron_pid = fork();
  if(matron_pid < 0) {
	printf("fork() returned an error\n");
	return 1;
  }
  
  if(matron_pid == 0) {
	run_matron();
	goto child_exit; // shouldn't get here
  }
  
  // parent continues.
  // fork child process to run crone
  crone_pid = fork();
  if(crone_pid < 0) {
	printf("fork() returned an error\n");
	return 1;
  }
  
  if(crone_pid == 0) {
	run_crone();
	goto child_exit; // shouldn't get here
  }

  // parent continues. 
  // close unused pipe endpoints
  close(matron_pipe_in[PIPE_READ]);
  close(matron_pipe_out[PIPE_WRITE]);
  close(crone_pipe_out[PIPE_WRITE]);

  // create threads to handle the children's i/o
  launch_thread(&matron_tx_tid, &matron_tx_loop, NULL);
  launch_thread(&matron_rx_tid, &matron_rx_loop, NULL);
  launch_thread(&crone_rx_tid, &crone_rx_loop, NULL);
  return 0;
  
 child_exit:
  perror("child process quit unexpectedly");
  return -1;
}

int io_deinit(void) {
  quit_crone();
  kill(matron_pid, SIGUSR1);
  pthread_cancel(matron_rx_tid);
  pthread_cancel(matron_tx_tid);
  cleanup_pipe(matron_pipe_in);
  cleanup_pipe(matron_pipe_out);
  cleanup_pipe(crone_pipe_out);
}

void io_send_code(char* buf, size_t len) {  
  write(matron_pipe_in[PIPE_WRITE], buf, len);
  write(matron_pipe_in[PIPE_WRITE], "\n", 1);
}

int io_loop(void) {
  // wait for threads to exit
  pthread_join(matron_tx_tid, NULL);
  pthread_join(matron_rx_tid, NULL);
  pthread_join(crone_rx_tid, NULL);
  // wait for child processes to exit
  waitpid(matron_pid, NULL, 0);
  waitpid(crone_pid, NULL, 0);
  printf("\n\nfare well\n\n");
  return 0;
}

int run_matron(void) {
  if(matron_pid != 0) {
	printf("error: calling run_matron() from parent process\n");
	return -1;
  }
  // redirect stdin
  if (dup2(matron_pipe_in[PIPE_READ], STDIN_FILENO) == -1) {
	perror("redirecting stdin");
	return -1;
  }

  // redirect stdout
  if (dup2(matron_pipe_out[PIPE_WRITE], STDOUT_FILENO) == -1) {
	perror("redirecting stdout");
	return -1;
  }

  // redirect stderr
  if (dup2(matron_pipe_out[PIPE_WRITE], STDERR_FILENO) == -1) {
	perror("redirecting stderr");
	return -1;
  }

  close(matron_pipe_in[PIPE_READ]);
  close(matron_pipe_in[PIPE_WRITE]);
  close(matron_pipe_out[PIPE_READ]);
  close(matron_pipe_out[PIPE_WRITE]); 

  char *argv[] = { "matron", "57120", "8888", NULL};
  char *env[] = { NULL };
	
  execv("matron/matron", argv);
  perror("execv"); // shouldn't get here
}

void* matron_rx_loop(void* psock) {
  char rxbuf[PIPE_BUF_SIZE];
  int len;
  while(1) {
	len = read(matron_pipe_out[PIPE_READ], rxbuf, PIPE_BUF_SIZE-1);
	if(len > 0) {
	  rxbuf[len] = '\0';
	  ui_matron_line(rxbuf, len);
	}	
  }
}

void* matron_tx_loop(void* x) {
  // wait a bit for the child executable
  usleep(100000);
  ui_loop();
  // quit when ui exits
  io_deinit();
}

void run_crone(void) {
  // TODO
  while(1) { usleep(1000000); }
}

void quit_crone(void) {
  kill(crone_pid, SIGUSR1);
  pthread_cancel(crone_rx_tid);
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

void cleanup_pipe(int pipe[2]) {
  close(pipe[PIPE_READ]);
  close(pipe[PIPE_WRITE]);
}
