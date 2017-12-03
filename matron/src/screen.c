/*
 * screen.c
 *
 * oled
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h> 
#include <sys/ioctl.h>

int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;

void screen_init(void) {
    // Open the file for reading and writing
    fbfd = open("/dev/fb1", O_RDWR);
    if (fbfd == -1) {
        printf("Error: cannot open framebuffer device.\n");
    }
    printf("The framebuffer device was opened successfully.\n");

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
    }
    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, 
                 vinfo.bits_per_pixel );

    // map fb to user mem 
    screensize = finfo.smem_len;
    fbp = (char*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        printf("Failed to mmap.\n");
    } 
}


void screen_deinit(void) {
  // unmap fb file from memory
  munmap(fbp, screensize);
  // close fb file
  close(fbfd);
}


void screen_pixel(int x, int y, int z) {
  memset(fbp+((y*128+x)*4),z*16,4);
}
