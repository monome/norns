#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "events.h"
#include "repl.h"
#include "weaver.h"

static char *rxbuf;
static size_t rxlen = 0;
static int quit = 0;
static pthread_t pid;

pthread_t repl_get_pid() { return pid; }

static void* repl_run(void* p) {
  while(!quit) { 
	getline(&rxbuf, &rxlen, stdin);
  	rxbuf[strlen(rxbuf)-1] = '\0';
	if( (strcmp(rxbuf,"quit") == 0)
		|| (strcmp(rxbuf,"exit") == 0)
		|| (strcmp(rxbuf,"q") == 0)
		|| (strcmp(rxbuf,"bye") == 0)
		) {
	  quit = 1;
	  // tell main event loop to quit
	  event_t ev = EVENT_QUIT;
	  event_post(ev, NULL, NULL);
	  printf("repl says bye \r\n");
	}
	else {
	  w_run_code(rxbuf);
	  // FIXME: somehow need to redirect lua output completely.
	  // see issue page on github
	  //	  printf("%s\n", l_get_stack_buf());
	}
  }
}

void repl_loop(void) {
  pthread_attr_t attr;
  pid = pthread_create(&pid, &attr, &repl_run, NULL);
  
}


#undef REPL_RX_BUF_SIZE
//#undef REPL_TX_BUF_SIZE
