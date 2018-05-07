#include <stdio.h>
#include <unistd.h>
#include <lo/lo.h>

 int pings;
int done = 0;
 
 int start_handler(const char* path, const char* types, lo_arg ** argv,
		   int argc, void *data, void *user_data) {
   pings = 0;
 }

 int ping_handler(const char* path, const char* types, lo_arg ** argv,
		  int argc, void *data, void *user_data) {
   pings += 1;
 }

 int end_handler(const char* path, const char* types, lo_arg ** argv,
		 int argc, void *data, void *user_data) {
   fprintf(stderr, "received %d pings \n", pings);
   done = 1;
 }



void error(int num, const char *m, const char *path) {
  fprintf(stderr, "error creating server thread\n");
}

int main(int argc, char *argv[]) {

  lo_server_thread st = lo_server_thread_new("57120", error);

  lo_server_thread_add_method(st, "/test/start", NULL, start_handler, NULL);
  lo_server_thread_add_method(st, "/test/ping", NULL, ping_handler, NULL);
  lo_server_thread_add_method(st, "/test/end", NULL, end_handler, NULL);

  lo_server_thread_start(st);    
  while(!done) { ;; } 
  lo_server_thread_free(st);
  
  return 0;
}
