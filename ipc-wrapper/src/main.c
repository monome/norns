/*
 * ipc-wrapper
 *
 * this utility launches an arbitrary executable as a child process,
 * and binds the child's standard I/O to a pair of IPC sockets.
 *
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>
#include <unistd.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>
#include <readline/readline.h>
#include <readline/history.h>

#define PIPE_READ 0
#define PIPE_WRITE 1
#define PIPE_BUF_SIZE 4096

pid_t child_pid;
int pipe_rx[2];
int pipe_tx[2];

pthread_t tid_rx, tid_tx;

bool needs_rx;
bool needs_tx;

int sock_rx, sock_tx;
int eid_rx, eid_tx;

void quit(void) {
  nn_shutdown(sock_rx, eid_rx);
  nn_shutdown(sock_tx, eid_tx);
}

void bind_sock(int *sock, int *eid, char *url) {
  *sock = nn_socket(AF_SP, NN_BUS);
  printf("attempting to bind socket at url %s\n", url);
  assert ( ( *eid = nn_bind(*sock, url) ) >= 0 );
  /* assert(*eid >= 0); */
}

void *loop_rx(void *p) {
  (void)p;
  int nb;
  while(1) {
    char *buf = NULL;
    nb = nn_recv(sock_rx, &buf, NN_MSG, 0);
    write(pipe_rx[PIPE_WRITE], buf, nb);
    nn_freemsg(buf);
  }
}

void *loop_tx(void *p) {
  (void)p;
  char buf[PIPE_BUF_SIZE];
  int nb;
  while(1) {
    nb = read(pipe_tx[PIPE_READ], buf, PIPE_BUF_SIZE - 1);
    if(nb > 0) {
      buf[nb] = '\0';
      printf(buf);
      nn_send(sock_tx, buf, nb, 0);
    }
  }
}

void launch_thread(pthread_t *tid, void *(*start_routine)(void *), void *data) {
  pthread_attr_t attr;
  int s;
  s = pthread_attr_init(&attr);
  if(s) { printf("error initializing thread attributes \n"); }
  s = pthread_create(tid, &attr, start_routine, data);
  if(s) { printf("error creating thread\n"); }
  pthread_attr_destroy(&attr);
}

int launch_exe( int argc,  char **argv) {
  (void)argc;
  char *exe = argv[1];
  char *url_rx = argv[2];
  char *url_tx = argv[3];
  needs_rx = (strcmp(url_rx, "NONE") != 0);
  needs_tx = (strcmp(url_tx, "NONE") != 0);

  if(!needs_rx) { printf("skipping rx socket\n"); }
  if(!needs_tx) { printf("skipping tx socket\n"); }

  // create pipes
  if(needs_rx) {
    if(pipe(pipe_rx) < 0) {
      perror("allocating pipe for input redirect");
      return -1;
    }
  }

  if(needs_tx) {
    if(pipe(pipe_tx) < 0) {
      perror("allocating pipe for output redirect");
      return -1;
    }
  }

  // fork the child process
  child_pid = fork();
  if(child_pid < 0) {
    printf("fork() returned an error\n");
    return 1;
  }

  if(child_pid == 0) {
    // child continues...

    // copy i/o to pipes
    if(needs_rx) {
      if (dup2(pipe_rx[PIPE_READ], STDIN_FILENO) == -1) {
        perror("redirecting stdin");
        return -1;
      }
      close(pipe_rx[PIPE_READ]);
      close(pipe_rx[PIPE_WRITE]);
    }

    if(needs_tx) {
      if (dup2(pipe_tx[PIPE_WRITE], STDOUT_FILENO) == -1) {
        perror("redirecting stdout");
        return -1;
      }
      if (dup2(pipe_tx[PIPE_WRITE], STDERR_FILENO) == -1) {
        perror("redirecting stderr");
        return -1;
      }
      close(pipe_tx[PIPE_READ]);
      close(pipe_tx[PIPE_WRITE]);
    }

    // launch the child executable
    char **child_argv = &(argv[4]);
    execv(exe, child_argv);
    perror("execv"); // shouldn't get here
  } else {
    // parent continues...

    // close unused pipe endpoints
    close(pipe_rx[PIPE_READ]);
    close(pipe_tx[PIPE_WRITE]);

    // set up sockets and threads
    if(needs_rx) {
      bind_sock(&sock_rx, &eid_rx, url_rx);
      launch_thread(&tid_rx, &loop_rx, NULL);
    }

    if(needs_tx) {
      bind_sock(&sock_tx, &eid_tx, url_tx);
      launch_thread(&tid_tx, &loop_tx, NULL);
    }
  }

  // wait for the child process to exit
  int wpid, status;
  do {
    wpid = waitpid(child_pid, &status, WUNTRACED | WCONTINUED );
    if (wpid == -1) {
      perror("waitpid");
      exit(EXIT_FAILURE);
    }
    if ( WIFEXITED(status) ) {
      printf( "child exited, status=%d\n", WEXITSTATUS(status) );
    } else if ( WIFSIGNALED(status) ) {
      printf( "child killed (signal %d)\n", WTERMSIG(status) );
    } else if ( WIFSTOPPED(status) ) {
      printf( "child stopped (signal %d)\n", WSTOPSIG(status) );
    } else if ( WIFCONTINUED(status) ) {
      printf("child continued\n");
    } else {
      printf("unexpected status (0x%x)\n", status);
    }
  } while ( !WIFEXITED(status) && !WIFSIGNALED(status) );

  return 0;
}

int main( int argc,  char **argv) {
  if(argc < 4) {
    printf(
      "usage: ipc-wrapper BINARY IN_SOCKET OUT_SOCKET BINARY <child args...>");
  }

  launch_exe(argc, argv);
}
