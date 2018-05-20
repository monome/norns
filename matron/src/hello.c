#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "screen.h"

#define LIGHTS 32
#define GRAVITY 2048

struct {
    double x;
    double y;
    double dx;
    double dy;
    int range;
    int life;
} light[LIGHTS];

struct {
    double x;
    double y;
    double dx;
    double dy;
} center;

struct {
    int x;
    int y;
} black;

static int count = 0;
static int start = 1;

void norns_hello_init() {
    srand(time(NULL));
    screen_aa(0);
    for(int i=0;i<LIGHTS;i++)
        light[i].range = 1;
    black.x = 60 + rand()%8;
    black.y = 28 + rand()%8;
    center.x = 60 + rand()%8;
    center.y = 28 + rand()%8;
    center.dx = 0;
    center.dy = 0; 
 }

int norns_hello(int live) {
    count++;

    if(count == 256) {
        black.x = 60 + rand()%8;
        black.y = 28 + rand()%8;
        count = 0;
    }

    screen_clear();
    //screen_line_width(1.0); // FIXME: for some reason setting this disables drawing
    
    center.dx = center.dx + (black.x - center.x)/GRAVITY;
    center.dy = center.dy + (black.y - center.y)/GRAVITY;
    center.x = center.x + center.dx;
    center.y = center.y + center.dy;

    //screen_move(center.x,center.y);
    //screen_line_rel(1,0);
    //screen_level(15);
    //screen_stroke();
    
    int alive = 0;

    for(int i=0;i<LIGHTS;i++) {
        if(light[i].range == 2) {
            if(start<64) start++; 
            light[i].range--;
        } else if(light[i].range > 2) {
            light[i].range--;
            light[i].x += light[i].dx;
            light[i].y += light[i].dy; 
            alive++;
            screen_move(light[i].x,light[i].y);
            screen_line_rel(1,0);
            screen_level(ceil(15*light[i].range/light[i].life)*(start/64.0));
            screen_stroke();
        } else if(live) { 
            light[i].life = 64 + rand()%64;
            light[i].range = light[i].life;
            light[i].x = center.x;
            light[i].y = center.y;
            light[i].dx = (rand()%32-16)/96.0;
            light[i].dy = (rand()%32-16)/96.0;
        } 
    }
    screen_update();

    return alive;
}
