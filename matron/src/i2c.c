#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ADDR_HP 0x60
#define ADDR_OUT 0x28
#define ADDR_IN 0x29

void i2c_init(void) {
    int file;
    char filename[40];
    int addr = ADDR_HP;


    sprintf(filename,"/dev/i2c-1");
    if ((file = open(filename,O_RDWR)) < 0) {
        printf("Failed to open the bus.");
        exit(1);
    }

    if (ioctl(file,I2C_SLAVE,addr) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        exit(1);
    }

    char buf[10] = {0};
    //float data;
    //char channel;
/*
    for(int i = 0; i<4; i++) {
        // Using I2C Read
        if (read(file,buf,2) != 2) {
            // ERROR HANDLING: i2c transaction failed 
            printf("Failed to read from the i2c bus.\n");
            buffer = g_strerror(errno);
            printf(buffer);
            printf("\n\n");
        } else {
            data = (float)((buf[0] & 0b00001111)<<8)+buf[1];
            data = data/4096*5;
            channel = ((buf[0] & 0b00110000)>>4);
            printf("Channel %02d Data:  %04f\n",channel,data);
        }
    }
*/
    // enable hp
    buf[0] = 1;
    buf[1] = 192;

    if (write(file,buf,2) != 2) {
        printf("Failed to write to the i2c bus.\n");
    }
}
