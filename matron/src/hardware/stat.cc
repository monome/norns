/*
 * stat.c
 *
 * hardware status monitoring
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include "events.h"

#define STAT_INTERVAL 2

// SoC temperature sensor, value in millidegrees Celsius
#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"

static pthread_t p;
static bool have_temp;

void *stat_check(void *);

void stat_init() {
    if (!(have_temp = access(TEMP_PATH, R_OK) == 0)) {
        fprintf(stderr, "Unable to check temperature: " TEMP_PATH " not readable\n");
    }

    if (pthread_create(&p, NULL, stat_check, 0)) {
        fprintf(stderr, "STAT: Error creating thread\n");
    }
    pthread_setname_np(p, "stat_check");
}

void stat_deinit() {
    pthread_cancel(p);
}

void *stat_check(void *x) {
    (void)x;
    int number = -1;
    int disk = 0;
    int temp = 0;
    int cpu[5] = {0, 0, 0, 0, 0};

    uint32_t user, nice, system, idle, iowait, irq, softirq, steal;
    uint32_t sumidle = 0, sumnonidle = 0, total = 0;
    uint32_t prevsumidle[5] = {0, 0, 0, 0, 0};
    uint32_t prevtotal[5] = {0, 0, 0, 0, 0};
    int32_t totald, idled;

    while (1) {
        number++;
        if (number == 5)
            number = 0;

        // check disk every 5 sleeps
        if (number == 0) {
            struct statvfs vfs;
            if (statvfs("/", &vfs) == 0) {
                disk = (int)((uint64_t)vfs.f_bavail * vfs.f_frsize / 1024000); // MB, legacy df scale
            } else {
                fprintf(stderr, "Error: disk free read\n");
            }
        }

        // check temp every 5
        if (have_temp && number == 0) {
            FILE *tf = fopen(TEMP_PATH, "r");
            if (tf == NULL) {
                fprintf(stderr, "Error: temp read\n");
            } else {
                int milli;
                if (fscanf(tf, "%d", &milli) == 1) {
                    temp = milli / 1000;
                } else {
                    fprintf(stderr, "Error: temp read\n");
                }
                fclose(tf);
            }
        }

        // check cpu
        FILE *f = fopen("/proc/stat", "r");
        if (f == NULL) {
            fprintf(stderr, "Error: cpu read\n");
        } else {
            char label[16];
            for (int i = 0; i < 5; i++) {
                if (fscanf(f, "%15s %u %u %u %u %u %u %u %u", label, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 9 || strncmp(label, "cpu", 3) != 0) {
                    break;
                }
                int ch;
                while ((ch = fgetc(f)) != EOF && ch != '\n') {
                }

                sumidle = idle + iowait;
                sumnonidle = user + nice + system + irq + softirq + steal;
                total = sumnonidle + sumidle;
                totald = total - prevtotal[i];
                idled = sumidle - prevsumidle[i];
                cpu[i] = totald > 0 ? 100 * (totald - idled) / totald : 0;
                prevsumidle[i] = sumidle;
                prevtotal[i] = total;
            }
            fclose(f);
        }

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
    return NULL;
}
