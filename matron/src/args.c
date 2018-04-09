#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG_BUF_SIZE 64

struct args {
    char loc_port[ARG_BUF_SIZE];
    char rem_port[ARG_BUF_SIZE];
    char scsynth_port[ARG_BUF_SIZE];
};

static struct args a = {
    .loc_port = "8888",
    .rem_port = "57120",
    .scsynth_port = "57110",
};

int args_parse(int argc, char **argv)
{
    int opt;
    while( ( opt = getopt(argc, argv, "r:l:s:m:") ) != -1 ) {
        switch(opt) {
        case 'l':
            strncpy(a.loc_port, optarg, ARG_BUF_SIZE);
            break;
        case 'r':
            strncpy(a.rem_port, optarg, ARG_BUF_SIZE);
            break;
        case 's':
            strncpy(a.scsynth_port, optarg, ARG_BUF_SIZE);
            break;
        default:
            ;;
        }
    }
    return 0;
}

const char *args_local_port(void) {
    return a.loc_port;
}

const char *args_remote_port(void) {
    return a.rem_port;
}

const char *args_scsynth_port(void) {
    return a.scsynth_port;
}
