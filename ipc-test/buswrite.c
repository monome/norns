
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
  
  while(1) {
	char* msg = readline("> ");
	size_t sz = strlen(msg) + 2;
	char* msgcat = calloc(sz, sizeof(char));
	snprintf(msgcat, sz, "%s\n", msg);	
	printf("sending to socket %d: %s (%dB)\n", sock, msgcat, sz);
	int tx = nn_send (sock, msgcat, sz, 0);
	assert(tx == sz);
	printf("sent\n");
	free(msg);
	free(msgcat);
  }
}

int main (const int argc, const char **argv) {
  node(argc, argv);
}
