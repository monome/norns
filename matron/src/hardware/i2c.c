/* i2c.c
 *
 * hp driver: TPA6130A2
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "events.h"
#include "i2c.h"
#include "platform.h"

#define ADDR_HP 0x60
#define ADDR_ADC 0x1D

static int file;
static char buf[10];

static pthread_t p;

static int pos[3] = {0,0,0};
// division of pot 0-1 range for ticks, TH is for wrap detection
#define DIV 64
#define DIVTH 32
#define ADC_RATE 30000

void *adc_read(void *);

void i2c_init(void) {
	if (platform() != PLATFORM_CM3) return;
	char filename[40];

	fprintf(stderr, "i2c init...\n");

	sprintf(filename, "/dev/i2c-1");
	if ((file = open(filename, O_RDWR | O_NONBLOCK)) < 0) {
		fprintf(stderr, "ERROR (i2c) failed to open bus\n");
		return;
	}

	// HP
	if (ioctl(file, I2C_SLAVE, ADDR_HP) < 0) {
		fprintf(stderr, "ERROR (i2c) failed to acquire bus access and/or talk to device\n");
		return;
	}
	buf[0] = 1; // reg for settings p21
	buf[1] = 192;
	if (write(file, buf, 2) != 2) {
		fprintf(stderr, "ERROR (i2c/hp) failed to write\n");
		return;
	}

	// ADC
	if (ioctl(file, I2C_SLAVE, ADDR_ADC) < 0) {
		fprintf(stderr, "(i2c) ADC connect fail\n");
		return;
	}

	buf[0] = 8; // 0-5 only
	buf[1] = 0xC0;
	if (write(file, buf, 2) != 2) {
		fprintf(stderr, "ERROR (i2c/adc) failed to write\n");
		return;
	}
	buf[0] = 7; // continuous
	buf[1] = 1;
	if (write(file, buf, 2) != 2) {
		fprintf(stderr, "ERROR (i2c/adc) failed to write\n");
		return;
	}
	buf[0] = 0xB;     // ext vref
	buf[1] = 1;     
	if (write(file, buf, 2) != 2) {
		fprintf(stderr, "ERROR (i2c/hp) failed to write\n");
		return;
	}
	buf[0] = 0;     // config, p20
	buf[1] = 1;     // start
	if (write(file, buf, 2) != 2) {
		fprintf(stderr, "ERROR (i2c/adc) failed to write\n");
		return;
	}

	if (pthread_create(&p, NULL, adc_read, 0)) {
		fprintf(stderr, "i2c/ADC: Error creating thread\n");
	}
}

void i2c_deinit() {
}

void i2c_hp(int level) {
	if (platform() != PLATFORM_CM3) return;

	if (level < 0) {
		level = 0;
	} else if (level > 63) {
		level = 63;
	}

	if (ioctl(file, I2C_SLAVE, ADDR_HP) < 0) {
		fprintf(stderr, "ERROR (i2c) failed to acquire bus access and/or talk to slave\n");
		return;
	}
	buf[0] = 2; // reg for set level p17
	buf[1] = level;
	if (write(file, buf, 2) != 2) {
		fprintf(stderr, "ERROR (i2c/hp) failed to write\n");
		return;
	}
}


float angle_360(float a, float b){
	// interpolant coefficient
	float c;
	if(b > 0.5){
		c = 2.0 * (1.0 - b);
	} else {
		c = b * 2.0;
	}

	// shift quadrants into (0..1) range
	float aa = a;
	float bb = b;
	// flips
	if(b > 0.5){
		aa = 1.0 - aa;
	}
	if(a < 0.5){
		bb = 1.0 - bb;
	}
	// shifts
	if(b > 0.5){
		aa += 0.5;
	} else if(a > 0.5){
		aa -= 0.5;
	} else{ // a < 0.5
		aa += 1.5;
	}
	if(a < 0.5){
		bb += 1.0;
	}
	// down-scale
	aa *= 0.5;
	bb *= 0.5;

	// linear interpolate
	return aa +  c*(bb-aa);
}


void *adc_read(void *x) {
	(void)x;
	int now[6];
	const int reorder[3] = {1,3,2};

	fprintf(stderr, "(i2c) ADC thread start...\n");

	usleep(100000); // settling time before first read

	if (ioctl(file, I2C_SLAVE, ADDR_ADC) < 0) {
		fprintf(stderr, "(i2c) ADC connect fail\n");
	}

	// read initial positions
	for(int i=0;i<6;i++) {
		buf[0] = 0x20 + i;
		if (write(file, buf, 1) != 1) {
			fprintf(stderr, "ERROR (i2c/adc) failed to write\n");
			break;
		}
		read(file, buf, 2);
		now[i] = ((buf[0]<<4) + (buf[1]>>4));
	}
	for(int i=0;i<3;i++) { 
		pos[i] = (int)DIV*angle_360((float)now[i*2]/4096,(float)now[i*2+1]/4096);
	}

	while(1) {
		if (ioctl(file, I2C_SLAVE, ADDR_ADC) < 0) {
			fprintf(stderr, "(i2c) ADC connect fail\n");
		}
		for(int i=0;i<6;i++) {
			buf[0] = 0x20 + i;
			if (write(file, buf, 1) != 1) {
				fprintf(stderr, "ERROR (i2c/adc) failed to write\n");
				break;
			}

			read(file, buf, 2);
			//fprintf(stderr, "%x\t%x\t\t",buf[0],buf[1]);
			now[i] = ((buf[0]<<4) + (buf[1]>>4));
		}
		for(int i=0;i<3;i++) {
			int n = (int)DIV*angle_360((float)now[i*2]/4096,(float)now[i*2+1]/4096);
			int d = pos[i] - n;
			if(d>(DIV-DIVTH)) d -= DIV; // for rollover
			else if(d<(DIVTH-DIV)) d += DIV;

			if(d) {
				pos[i] = n;
				//fprintf(stderr, "%d %d\n",reorder[i],d);
				union event_data *ev = event_data_new(EVENT_ENC);
				ev->enc.n = reorder[i];
				ev->enc.delta = d;
				event_post(ev);
			}

			//fprintf(stderr, "%f\t", angle_360((float)adc[i*2]/4096,(float)adc[i*2+1]/4096));
		}

		usleep(ADC_RATE);
	}
}
