#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "events.h"
#include "repl.h"

static char *rxbuf;
static int quit = 0;
static pthread_t pid;

static void* repl_run(void* p) {
  size_t len, dum;
  char* code;
  printf("waiting for REPL input on stdin\n");
  printf("('q' quits)\n");
  while(!quit) {
	//	printf("\n> "); fflush(stdout);
	getline(&rxbuf, &dum, stdin);
	len = strlen(rxbuf);
	if(len == 2) {
	  if(rxbuf[0] == 'q') { 
		quit = 1;
		// tell main event loop to quit
		event_t ev = EVENT_QUIT;
		event_post(ev, NULL, NULL);
		printf("REPL has exited \r\n");
	  }
	}
	else if (len > 0) {
	  // event handler must free this chunk!
	  code = malloc((len+1) * sizeof(char));
	  strncpy(code, rxbuf, len);
	  code[len] = '\0';
	  event_post(EVENT_EXEC_CODE, code, NULL);
	}
  }
  free(rxbuf);
}

void repl_loop(void) {
   pthread_attr_t attr;
   int s;
   s = pthread_attr_init(&attr);
   if(s != 0) { printf("error in pthread_attr_init(): %d\n", s); }
   s = pthread_create(&pid, &attr, &repl_run, NULL);
   if(s != 0) { printf("error in pthread_create(): %d\n", s); }
   pthread_attr_destroy(&attr);
}
