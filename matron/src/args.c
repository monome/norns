#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct args {
  char loc_port[16];
  char rem_port[16];
};

static struct args a = {
  .loc_port = "8888",
  .rem_port = "57120"
};

int parse_args(int argc, char** argv)
{
  int flags, opt;
  while( (opt = getopt(argc, argv, "r:l:")) != -1 ) {
	switch(opt) {
	case 'l':
	  strncpy(a.loc_port, optarg, 16);
	  break;
	case 'r':	   
	  strncpy(a.rem_port, optarg, 16);
	  break;
	default:
	  ;;
	}
  }
  //  printf("local port: %s ; remote port: %s \n", a.loc_port, a.rem_port);
}


const char* args_local_port(void) {
  return a.loc_port;
}

const char* args_remote_port(void) {
  return a.rem_port;
}
