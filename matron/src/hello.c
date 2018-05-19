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
    light.r = 0;
}

void norns_hello() {
    screen_clear();
        screen_level(15);
    screen_move(20,20 + rand()%20);

    if(light.r) {
        light.r = 0;
    } else {
        light.r = 1;
    } 
    screen_text("norns");
    screen_update();
}
