#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if MATRON_INPUT_USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <pthread.h>

#include "events.h"
#include "input.h"

static pthread_t pid;

static void* input_run(void* p) {
  int quit = 0;
  size_t len, dum;
  char* rxbuf = (char*)NULL;
  char* line;
  while(!quit) {
  	getline(&rxbuf, &dum, stdin);
  	len = strlen(rxbuf);
  	if(len == 2) {
  	  if(rxbuf[0] == 'q') {
  		// tell main event loop to quit
  		event_t ev = EVENT_QUIT;
  		event_post(ev, NULL, NULL);
  		fflush(stdout);
  		quit = 1;
  		continue;
  	  }
  	}
  	if (len > 0) {
  	  // event handler must free this chunk!
  	  line = malloc((len+1) * sizeof(char));
  	  strncpy(line, rxbuf, len);
  	  line[len] = '\0';
  	  event_post(EVENT_EXEC_CODE_LINE, line, NULL);
  	}
  }
  free(rxbuf);
}

void input_init(void) {
   pthread_attr_t attr;
   int s;
   s = pthread_attr_init(&attr);
   if(s != 0) { printf("input_init(): error in pthread_attr_init(): %d\n", s); }
   s = pthread_create(&pid, &attr, &input_run, NULL);
   if(s != 0) { printf("input_init(): error in pthread_create(): %d\n", s); }
   pthread_attr_destroy(&attr);
}
