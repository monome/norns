#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARG_BUF_SIZE 64

struct args {
    char loc_port[ARG_BUF_SIZE];
    char ext_port[ARG_BUF_SIZE];
    char remote_port[ARG_BUF_SIZE];
    char crone_port[ARG_BUF_SIZE];
    char framebuffer[ARG_BUF_SIZE];
};

static struct args a = {
    .loc_port = "8888",
    .ext_port = "57120",
    .crone_port = "9999",
    .remote_port = "10111",
    .framebuffer = "/dev/fb0",
};

int args_parse(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "o:e:l:c:f:h")) != -1) {
        switch (opt) {
        case 'l':
            snprintf(a.loc_port, ARG_BUF_SIZE, "%s", optarg);
            break;
        case 'e':
            snprintf(a.ext_port, ARG_BUF_SIZE, "%s", optarg);
            break;
        case 'o':
            snprintf(a.remote_port, ARG_BUF_SIZE, "%s", optarg);
            break;
        case 'c':
            snprintf(a.crone_port, ARG_BUF_SIZE, "%s", optarg);
            break;
        case 'f':
            snprintf(a.framebuffer, ARG_BUF_SIZE, "%s", optarg);
            break;
        case '?':
        case 'h':
        default:
            fprintf(stdout, "Start matron with optional overrides:\n");
            fprintf(stdout, "-l   override OSC local port [default %s]\n", a.loc_port);
            fprintf(stdout, "-e   override OSC ext port [default %s]\n", a.ext_port);
            fprintf(stdout, "-o   override OSC remote port [default %s]\n", a.remote_port);
            fprintf(stdout, "-c   override crone port [default %s]\n", a.crone_port);
            fprintf(stdout, "-f   override framebuffer path [default %s]\n", a.framebuffer);
            exit(1);
            ;
        }
    }
    return 0;
}

const char *args_local_port(void) {
    return a.loc_port;
}

const char *args_ext_port(void) {
    return a.ext_port;
}

const char *args_remote_port(void) {
    return a.remote_port;
}

const char *args_crone_port(void) {
    return a.crone_port;
}

const char *args_framebuffer_path(void) {
    return a.framebuffer;
}
