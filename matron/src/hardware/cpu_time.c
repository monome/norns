#include "cpu_time.h"

#include <time.h>

struct timespec t0;

static struct timespec diff(const struct timespec *start, const struct timespec *end)
{
    struct timespec temp;
    if ((end->tv_nsec - start->tv_nsec)<0) {
        temp.tv_sec = end->tv_sec - start->tv_sec - 1;
        temp.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
    } else {
        temp.tv_sec = end->tv_sec - start->tv_sec;
        temp.tv_nsec = end->tv_nsec - start->tv_nsec;
    }
    return temp;
}

void cpu_time_start() {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t0);
 }

unsigned long int cpu_time_get_delta_ns() {
    struct timespec t1;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
    struct timespec delta = diff(&t0, &t1);
    return (delta.tv_sec * 1000000000) + delta.tv_nsec;
 }