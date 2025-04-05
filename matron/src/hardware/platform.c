#include <unistd.h>
#include <stdlib.h>

#include "platform.h"
#include <string.h>
#include <stdio.h>

platform_t p;

void init_platform() {
  if(access("/sys/firmware/devicetree/base/model", F_OK ) != -1) {

	FILE* fptr = fopen("/sys/firmware/devicetree/base/model","r");
	char modelString[100];
	fgets(modelString, 100, fptr);
	fclose(fptr);

   if (strstr(modelString, "Compute Module 3")){
     p = PLATFORM_CM3;
   } else if (strstr(modelString, "Compute Module 4")){
     p = PLATFORM_CM4;   
   } else if (strstr(modelString, "Compute Module 4S")){
     p = PLATFORM_CM4S;   
   } else if (strstr(modelString, "Raspberry Pi 3")){
     p = PLATFORM_PI3;
   } else if (strstr(modelString, "Raspberry Pi 4")){
     p = PLATFORM_PI4;
   }
  } else {
    p = PLATFORM_OTHER;
  }
}

platform_t platform() {
  return p;
}
