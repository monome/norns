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

    d->fd = open(d->base.path, O_RDWR | O_NOCTTY | O_SYNC);

    if (d->fd < 0) {
        fprintf(stderr, "failed to open crow device: %s\n", d->base.path);
        return -1;
    }

    tcgetattr(d->fd,&d->oldtio);
    //d->newtio.c_cflag = CROW_BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
    d->newtio.c_cflag = CROW_BAUDRATE | CS8 | CLOCAL | CREAD;
    //d->newtio.c_iflag = IGNPAR | ICRNL;
    d->newtio.c_iflag &= ~(IXON | IXOFF | IXANY);
    d->newtio.c_oflag = 0;
    //d->newtio.c_lflag = ICANON;
    d->newtio.c_lflag = 0;
    d->newtio.c_cc[VMIN]=0;
    d->newtio.c_cc[VTIME]=5;
    tcflush(d->fd, TCIFLUSH);
    tcsetattr(d->fd,TCSANOW,&d->newtio);

    base->start = &dev_crow_start;
    base->deinit = &dev_crow_deinit;

    return 0;
}

static void handle_event(void *dev, uint8_t id) {
    union event_data *ev = event_data_new(EVENT_CROW_EVENT);
    ev->crow_event.dev = dev;
    ev->crow_event.id = id;
    event_post(ev);
}

void *dev_crow_start(void *self) {
	struct dev_crow *di = (struct dev_crow *)self;
  struct dev_common *base = (struct dev_common *)self;

	uint8_t len;

	while(1) {
		len = read(di->fd, di->line, 255);
		if(len > 0) {
			di->line[len] = 0; // add null to end of string
			if(len>1) {
				//fprintf(stderr,"crow> %s", di->line);
				handle_event(self, base->id);
			}
			len = 0;
		}
		usleep(100);
	}
	return NULL;
}

void dev_crow_deinit(void *self) {
    struct dev_crow *di = (struct dev_crow *)self;
  	tcsetattr(di->fd,TCSANOW,&di->oldtio);
}

void dev_crow_send(struct dev_crow *d, const char *line) {
	uint8_t i = 0;
  uint8_t wlen;
	char s[256];
	strcpy(s,line);
	strcat(s,"\n");
	//fprintf(stderr,"crow_send: %s",line);
  while(i !=strlen(s)) {
    wlen = write(d->fd, s+i, 1);
    if(wlen==1) i++;
  }
}
