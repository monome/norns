#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "screen.h"

struct {
    double x;
    double y;
    double dx;
    double dy;
    double r;
} light;

void norns_hello_init() {
    srand(time(NULL));
    light.r = 0
}

void norns_hello() {
    if(light.r) {

    } else {

    } 
}
