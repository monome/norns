#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

int matron_pipe_in[2];
int matron_pipe_out[2];

int crone_pipe_out[2];

// child process id's
pid_t matron_pid;
pid_t crone_pid;

// client thread id's
pthread_t matron_tx_tid, matron_rx_tid;
pthread_t crone_rx_tid;

#define BUFFER_SIZE 1024

// utility to launch a joinable thread
void launch_thread(pthread_t *tid, void *(*start_routine) (void *), void* data) {
  pthread_attr_t attr;
  int s;
  s = pthread_attr_init(&attr);
  if(s) { printf("error initializing thread attributes \n"); }
  s = pthread_create(tid, &attr, start_routine, data);
  if(s) { printf("error creating thread\n"); }
  pthread_attr_destroy(&attr);
}

// run the matron executable and redirect its i/o
int run_matron(void) {
  if(matron_pid != 0) {
	printf("error: calling run_matron() from parent process \n");
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
	perror("execv");
  
}

// matron rx thread
void* matron_rx(void* psock) {
  char rxbuf[BUFFER_SIZE];
  int nb;
  while(1) {
	// print server response
	nb = read(matron_pipe_out[PIPE_READ], rxbuf, BUFFER_SIZE-1);
	if(nb > 0) {
	  rxbuf[nb] = '\0';
	  printf("%s", rxbuf);
	}
	
  }
}

// matron tx thread
void* matron_tx(void* x) {
  char txbuf[BUFFER_SIZE];
  int res;
  int nb;
  int ch;
  int newline = 0;

  // wait a bit for the child executable
  usleep(100000);
  
  while(1) {
	// FIXME: weird issues with getline() and threads/children (?)
	// for now, doing this craziness instead
	nb = 0;
	txbuf[0] = '\0';
	newline = 0;
	//	printf("waiting for input\n");
	// read from stdin
	while(newline != 1) {
	  if(nb < (BUFFER_SIZE-1)) {
		ch = getchar();
		txbuf[nb++] = (char)ch;
		if(ch == 10) { newline = 1; }
	  } else {
		newline = 1;
	  }
	}
	txbuf[nb] = '\0';
	if(nb == 2 && txbuf[0] == 'q') {
	  printf("time to quit! (FIXME) \n");
	  // TODO: quit everything!
	}
	// send to server
	//	printf("sending %dB to server: \n%s\n", nb, txbuf);
	write(matron_pipe_in[PIPE_WRITE], txbuf, nb);
	//	printf("... sent\n");
  }
}

void run_crone(void) {
  // TODO
  while(1) { usleep(1000000); }
}


void* crone_rx(void* x) {
  // TODO
  while(1) { usleep(1000000); }
}


void cleanup_pipe(int pipe[2]) {
  close(pipe[PIPE_READ]);
  close(pipe[PIPE_WRITE]);
}

int main(int argc, char** argv) {

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

  // close unused pipe endpoints
  close(matron_pipe_in[PIPE_READ]);
  close(matron_pipe_out[PIPE_WRITE]);
  close(crone_pipe_out[PIPE_WRITE]);
  
  // create threads to handle the children's i/o
  launch_thread(&matron_tx_tid, &matron_tx, NULL);
  launch_thread(&matron_rx_tid, &matron_rx, NULL);
  launch_thread(&crone_rx_tid, &crone_rx, NULL);
  
  // wait for threads to exit
  pthread_join(matron_tx_tid, NULL);
  pthread_join(matron_rx_tid, NULL);
  pthread_join(crone_rx_tid, NULL);
 
  // wait for child processes to exit
  wait(0);
  
  printf("norns parent exit\n");
  return 0;

 child_exit:
  printf("norns child exit. this is an error! \n");
  return 0;
}

