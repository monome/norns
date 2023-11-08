#include "time_since.h"

#include <time.h>

struct timespec t0_cpu;
struct timespec t0_wall;

static struct timespec diff(const struct timespec *start, const struct timespec *end)
{
    struct timespec temp;
    if ((end->tv_nsec - start->tv_nsec)<0) {
        temp.tv_sec = end->tv_sec - start->tv_sec - 1;
        temp.tv_nsec = 1e9 + end->tv_nsec - start->tv_nsec;
    } else {
        temp.tv_sec = end->tv_sec - start->tv_sec;
        temp.tv_nsec = end->tv_nsec - start->tv_nsec;
    }
    return temp;
}

void cpu_time_start() {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t0_cpu);
 }

unsigned long int cpu_time_get_delta_ns() {
    struct timespec t1;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
    struct timespec delta = diff(&t0_cpu, &t1);
    return (delta.tv_sec * 1e9) + delta.tv_nsec;
 }

 void wall_time_start() {
    clock_gettime(CLOCK_MONOTONIC, &t0_wall);
 }

 unsigned long int wall_time_get_delta_ns() {
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    struct timespec delta = diff(&t0_wall, &t1);
    return (delta.tv_sec * 1e9) + delta.tv_nsec;
 }