#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "device_crow.h"
#include "events.h"

#define TEST_NULL_AND_FREE(p) if( (p) != NULL ) { free(p); } \
    else { fprintf(stderr, "error: double free in device_crow.c\n"); }

int dev_crow_init(void *self) {
    struct dev_crow *d = (struct dev_crow *)self;
    struct dev_common *base = (struct dev_common *)self;

    d->fd = open(d->base.path, O_RDWR | O_NOCTTY);

    if (d->fd < 0) {
        fprintf(stderr, "failed to open crow device: %s\n", d->base.path);
        return -1;
    }

    tcgetattr(d->fd,&d->oldtio);
    d->newtio.c_cflag = CROW_BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
    d->newtio.c_iflag = IGNPAR | ICRNL;
    d->newtio.c_oflag = 0;
    d->newtio.c_lflag = ICANON;
    d->newtio.c_cc[VMIN]=1;
    d->newtio.c_cc[VTIME]=0;
    tcflush(d->fd, TCIFLUSH);
    tcsetattr(d->fd,TCSANOW,&d->newtio);

    base->start = &dev_crow_start;
    base->deinit = &dev_crow_deinit;

    fprintf(stderr, "attached crow?\n");

    return 0;
}

static void handle_event(uint8_t id) {
    union event_data *ev = event_data_new(EVENT_CROW_EVENT);
    ev->crow_event.id = id;
    event_post(ev);
}

void *dev_crow_start(void *self) {
	struct dev_crow *di = (struct dev_crow *)self;

	uint8_t len;
	char buf[255];

	while(1) {
		len = read(di->fd, buf, 255);
		if(len > 0) {
			buf[len] = 0; // add null to end of string
			//fprintf(stderr,"%d\t > %s", len, buf);
			if(len>1) fprintf(stderr,"crow> %s", buf);
			len = 0;
			handle_event(len);
		}
		usleep(100);
	}
	return NULL;
}

void dev_crow_deinit(void *self) {
    struct dev_crow *di = (struct dev_crow *)self;
  	tcsetattr(di->fd,TCSANOW,&di->oldtio);
}

void dev_crow_send(struct dev_crow *d, char *line) {
	uint8_t i = 0;
  uint8_t wlen;
  while(i !=strlen(line)) {
    wlen = write(d->fd, line+i, 1);
    if(wlen==1) i++;
  }
}
