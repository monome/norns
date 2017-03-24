
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#define BUF_SIZE 4096

int node (const int argc, const char **argv) {
  int sock = nn_socket (AF_SP, NN_BUS);

  if(argc > 1) {
	if(nn_connect(sock, argv[1]) < 0) {
	  perror("error connecting");
	}
  }

  
  char msg[BUF_SIZE];
  while(1) {
	int nb = nn_recv (sock, msg, BUF_SIZE, 0);
	if(nb >=0) {
	  msg[nb] = '\0';
	  printf("%s", msg);
	}
	//	nn_freemsg (msg);
  }
}

int main (const int argc, const char **argv) {
  node(argc, argv);
}
