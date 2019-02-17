#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG_BUF_SIZE 64

struct args {
    char loc_port[ARG_BUF_SIZE];
    char ext_port[ARG_BUF_SIZE];
    char crone_port[ARG_BUF_SIZE];
};

static struct args a = {
    .loc_port = "8888",
    .crone_port = "9999",
    .ext_port = "57120",
};

int args_parse(int argc, char **argv)
{
    int opt;
    while( ( opt = getopt(argc, argv, "e:l:c:") ) != -1 ) {
        switch(opt) {
        case 'l':
            strncpy(a.loc_port, optarg, ARG_BUF_SIZE-1);
            break;
        case 'e':
            strncpy(a.ext_port, optarg, ARG_BUF_SIZE-1);
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

const char *args_ext_port(void) {
    return a.ext_port;
}

const char *args_crone_port(void) {
    return a.crone_port;
}
