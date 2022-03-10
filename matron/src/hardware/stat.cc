/*
 * stat.c
 *
 * hardware status monitoring
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "events.h"
#include "sidecar.h"

#define STAT_INTERVAL 2

static pthread_t p;
static bool have_vcgencmd;

void *stat_check(void *);

// extern def

void stat_init() {
    if (!(have_vcgencmd = system("which vcgencmd > /dev/null 2>&1") == 0)) {
        fprintf(stderr, "Unable to check temperature: vcgencmd not in path\n");
    }

#if 1 // debugging...
    if (pthread_create(&p, NULL, stat_check, 0)) {
        fprintf(stderr, "STAT: Error creating thread\n");
    }
#endif
}

void stat_deinit() {
    pthread_cancel(p);
}

void *stat_check(void *x) {
    (void)x;
    int number = -1;
    int disk = 0;
    int temp = 0;
    int cpu[5] = {0,0,0,0,0};

    char bufsub[8];

    uint32_t user, nice, system, idle, iowait, irq, softirq, steal;
    //uint32_t sumidle = 0, prevsumidle = 0, sumnonidle = 0, total = 0, prevtotal = 0;
    uint32_t sumidle = 0, sumnonidle = 0, total = 0;
    uint32_t prevsumidle[5] = {0,0,0,0,0};
    uint32_t prevtotal[5] = {0,0,0,0,0};
    int32_t totald, idled;

    while (1) {
        number++;
        if (number == 5)
            number = 0;

        // check disk every 5 sleeps
        if (number == 0) {
	    size_t size=0;
	    char *buff = NULL;
	    sidecar_client_cmd(&buff, &size, "df -l | grep '/dev/root' | awk '{print $4}'");
	    if(size==0) {
                fprintf(stderr, "Error: disk free read\n");
            } else {
                    disk = atoi(buff) / 1000; // convert to MB
            }
            free(buff);
        }

	// check temp every 5
        if (have_vcgencmd && number == 0) {
	    size_t size=0;
            char *buff = NULL;
	    sidecar_client_cmd(&buff, &size, "vcgencmd measure_temp");
	    if(size==0) {
                fprintf(stderr, "Error: temp read\n");
            } else {
                    bufsub[0] = buff[5];
                    bufsub[1] = buff[6];
                    bufsub[2] = 0;
                    temp = atoi(bufsub);
            }
	    free(buff);
        }


        // check cpu
	    size_t size=0;
        char *buff = NULL;
	    sidecar_client_cmd(&buff, &size,"cat /proc/stat");

        if(size==0) {
            fprintf(stderr, "Error: cpu read\n");
        } else {
            int i = 0;
            strtok(buff, " ");
            while(i<4) {
                user = atoi(strtok(NULL, " "));
                nice = atoi(strtok(NULL, " "));
                system = atoi(strtok(NULL, " "));
                idle = atoi(strtok(NULL, " "));
                iowait = atoi(strtok(NULL, " "));
                irq = atoi(strtok(NULL, " "));
                softirq = atoi(strtok(NULL, " "));
                steal = atoi(strtok(NULL, " "));
		//fprintf(stderr, "> %d %d %d %d %d %d %d %d\n", user, nice, system, idle, iowait, irq, softirq, steal);

                sumidle = idle + iowait;
                sumnonidle = user + nice + system + irq + softirq + steal;
                total = sumnonidle + sumidle;
                totald = total - prevtotal[i];
                idled = sumidle - prevsumidle[i];
                cpu[i] = 100 * (totald - idled) / totald;
                prevsumidle[i] = sumidle;
                prevtotal[i] = total;

                //fprintf(stderr,"%d --> %d\n", i, cpu);
		strtok(NULL, "\n");
		strtok(NULL, " ");
                i++;
            }
        }
        free(buff);

        // just send every tick
            union event_data *ev = event_data_new(EVENT_STAT);
            ev->stat.disk = disk;
            ev->stat.temp = temp;
            ev->stat.cpu = cpu[0];
            ev->stat.cpu1 = cpu[1];
            ev->stat.cpu2 = cpu[2];
            ev->stat.cpu3 = cpu[3];
            ev->stat.cpu4 = cpu[4];
            event_post(ev);

        sleep(STAT_INTERVAL);
    }
}
