#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "crone_main.h"
#include "matron_main.h"

#include "sidecar.h"

int main(int argc, char **argv) {
  int exec_name_size = strlen(argv[0]);

  if (fork() == 0) {
    strncpy(argv[0], "sidecar [norns]", exec_name_size);

    // child, set the thread name to sidecar for added clarity in gdb
    if (prctl(PR_SET_NAME, (unsigned long)"sidecar") < 0) {
      perror("prctl(PR_SET_NAME)");
    }

    // arrange to receive SIGHUP when the thread which forked the child exits
    if (prctl(PR_SET_PDEATHSIG, SIGHUP) < 0) {
      perror("prctl(PR_SET_PDEATHSIG)");
    }

    // detach this processes stdin from parent
    int fd = open("/dev/null", O_RDONLY);
    dup2(fd, 0);

    sidecar_server_main();
  } else {
    // parent
    sidecar_client_init();

    crone_main();
    matron_main(argc, argv);

    matron_cleanup();
    crone_cleanup();
  }
  return 0;
}