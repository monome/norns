/*
 * gpio.c
 *
 * keys and encoders
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "events.h"


int epfd;
int fd;
char buf;
struct epoll_event ev;
struct epoll_event events;
 
// extern def

void gpio_init() {
  int n;
  epfd = epoll_create(1);
  fd = open("/sys/class/gpio/gpio35/value", O_RDWR | O_NONBLOCK);
  printf("GPIO open returned %d: %s\n", fd, strerror(errno)); fflush(stdout);
  if(fd > 0) {
    buf = 0;

    ev.events = EPOLLET | EPOLLIN;
    ev.data.fd = fd;

    n = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    printf("GPIO epoll_ctl returned %d: %s\n", n, strerror(errno)); fflush(stdout); 
  }
}

void gpio_check() {
  int n;
  n = epoll_wait(epfd, &events, 1, 0); // wait for 1 event, 0ms
  //printf("GPIO epoll returned %d: %s\n", n, strerror(errno)); fflush(stdout);

  if(n > 0) {
    n = lseek(fd, 0, SEEK_SET);
    //printf("GPIO seek %d bytes: %s\n", n, strerror(errno)); fflush(stdout);
    n = read(fd, &buf, 1);
    //printf("GPIO read %d bytes: %s\n", n, strerror(errno)); fflush(stdout);
    printf("GPIO buf = 0x%x\n", buf);

    union event_data *ev = event_data_new(EVENT_GPIO);
    ev->gpio.pin = 35;
    ev->gpio.val = buf & 0x1;
    event_post(ev);
  }
}

