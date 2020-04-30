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

#define STAT_INTERVAL 2

// static int fd[3];
// static char buf[8];
static pthread_t p;

void *stat_check(void *);

// extern def

void stat_init() {
    if (pthread_create(&p, NULL, stat_check, 0)) {
        fprintf(stderr, "STAT: Error creating thread\n");
    }
}

void stat_deinit() {
    pthread_cancel(p);
}

void *stat_check(void *x) {
    (void)x;
    int number = -1;
    int disk = 0;
    int temp = 0;
    int cpu = 0;
    int _disk = -1;
    int _temp = -1;
    int _cpu = -1;

    FILE *fd;
    char buf[64];
    char bufsub[8];

    uint32_t user, nice, system, idle, iowait, irq, softirq, steal;
    uint32_t sumidle = 0, prevsumidle = 0, sumnonidle = 0, total = 0, prevtotal = 0;
    int32_t totald, idled;

    while (1) {
        number++;
        if (number == 5)
            number = 0;

        // check disk ever 5 sleeps
        if (number == 0) {
            if ((fd = popen("df -l | grep '/dev/root' | awk '{print $4}'", "r")) == NULL) {
                fprintf(stderr, "Error opening pipe: disk free read\n");
            } else {
                while (fgets(buf, 12, fd) != NULL) {
                    disk = atoi(buf) / 1000; // convert to MB
                                             // fprintf(stderr,"disk free: %d\n", disk);
                }
            }
            pclose(fd);
        }

        // check temp
        if ((fd = popen("vcgencmd measure_temp", "r")) == NULL) {
            fprintf(stderr, "Error opening pipe: temp read\n");
        } else {
            while (fgets(buf, 16, fd) != NULL) {
                strncpy(bufsub, buf + 5, 2);
                temp = atoi(bufsub);
                // fprintf(stderr,"temp: %d\r\n", temp);
            }
        }
        pclose(fd);

        // check cpu
        if ((fd = popen("head -n1 /proc/stat", "r")) == NULL) {
            fprintf(stderr, "Error opening pipe: cpu read\n");
        } else {
            while (fgets(buf, 64, fd) != NULL) {
                // fprintf(stderr,"%s\n", buf);
                strtok(buf, " ");
                user = atoi(strtok(NULL, " "));
                nice = atoi(strtok(NULL, " "));
                system = atoi(strtok(NULL, " "));
                idle = atoi(strtok(NULL, " "));
                iowait = atoi(strtok(NULL, " "));
                irq = atoi(strtok(NULL, " "));
                softirq = atoi(strtok(NULL, " "));
                steal = atoi(strtok(NULL, " "));

                prevsumidle = sumidle;
                sumidle = idle + iowait;
                sumnonidle = user + nice + system + irq + softirq + steal;
                prevtotal = total;
                total = sumnonidle + sumidle;
                totald = total - prevtotal;
                idled = sumidle - prevsumidle;
                cpu = 100 * (totald - idled) / totald;

                // fprintf(stderr,"%d\n", cpu);
            }
        }
        pclose(fd);

        if (_disk != disk || _temp != temp || _cpu != cpu) {
            _disk = disk;
            _temp = temp;
            _cpu = cpu;

            union event_data *ev = event_data_new(EVENT_STAT);
            ev->stat.disk = disk;
            ev->stat.temp = temp;
            ev->stat.cpu = cpu;
            event_post(ev);
        }

        sleep(STAT_INTERVAL);
    }
}
