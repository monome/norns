#include <stdio.h>
#include <unistd.h>
#include <lo/lo.h>

int count = 2000;

int main(int argc, char *argv[]) {

  lo_address t = lo_address_new("127.0.0.1", "57120");
  lo_send(t, "/test/start", NULL);
  usleep(20000);
  
  for(int i=0; i<count; ++i) {
    lo_send(t, "/test/ping", "i", i);
  }

  // this sleep is necessary...
  usleep(20000);
  lo_send(t, "/test/end", "");

  return 0;
}
