#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <jack/jack.h>

jack_client_t *client;
static int nxruns = 0;

static void signal_handler(int sig)
{
    jack_client_close(client);
    exit(0);
}

void jack_shutdown(void *arg)
{
     exit(1);
}

int xrun_callback(void* arg) {
    (void) arg;
    nxruns++; 
    return 0;
}

int get_xruns() { 
    int y = nxruns;
    nxruns = 0;
    return y;
}

int main(int argc, char *argv[])
{
    jack_options_t options = JackNullOption;
    jack_status_t status;
     client = jack_client_open ("jack_cpu_capture", options, &status);
    if (client == NULL) {
        fprintf(stderr, "jack_client_open() failed, "
                  "status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            fprintf(stderr, "Unable to connect to JACK server\n");
        }
        exit(1);
    }

    jack_on_shutdown(client, jack_shutdown, 0);
    jack_set_xrun_callback(client, &xrun_callback, 0);

    if (jack_activate(client)) {
        fprintf(stderr, "cannot activate client");
        exit(1);
    }
    
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);

    int n = 100;
    if (argc > 1) { 
        n = atoi(argv[1]);
    }

    float d = 0.25;
    if (argc > 2) { 
        d = atof(argv[2]);
    }

    long int p = (long int)(d*1000000.f);

    printf("# load, xruns\n");
    for (int i=0; i<n; ++i) { 
        printf("%f,\t%d\n", jack_cpu_load(client), get_xruns());
        usleep(p);
    }
    jack_client_close(client);
    exit(0);
}