#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "crone_main.h"
#include "matron_main.h"

#include "sidecar.h"

int main(int argc, char **argv) {
    int exec_name_size = strlen(argv[0]);

    int sync_pipe[2];
    if (pipe(sync_pipe) == -1) {
        perror("pipe");
        return 1;
    }

    if (fork() == 0) {
        close(sync_pipe[0]);

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

        sidecar_server_main(sync_pipe[1]);
    } else {
        close(sync_pipe[1]); // Parent closes write end

        // block here until the child writes a byte (meaning NNG listener is up)
        char b;
        read(sync_pipe[0], &b, 1);
        close(sync_pipe[0]);

        // parent
        sidecar_client_init();

        crone_main();
        matron_main(argc, argv);

        matron_cleanup();
        crone_cleanup();
        sidecar_client_cleanup();
    }
    return 0;
}