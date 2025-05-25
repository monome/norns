#include <unistd.h>
#include <stdlib.h>

#include "platform.h"
#include <string.h>
#include <stdio.h>

static platform_t p = PLATFORM_UNKNOWN;

void init_platform() {
  if(access("/sys/firmware/devicetree/base/model", F_OK ) != -1) {

    FILE* fptr = fopen("/sys/firmware/devicetree/base/model","r");
    char modelString[100];
    fgets(modelString, 100, fptr);
    fclose(fptr);

    if (strstr(modelString, "Compute Module 3")){
      p = PLATFORM_CM3;
    } else if (strstr(modelString, "Compute Module 4S")){
      p = PLATFORM_CM4S;
    } else if (strstr(modelString, "Compute Module 4")){
      p = PLATFORM_CM4;
    } else if (strstr(modelString, "Raspberry Pi 3")){
      p = PLATFORM_PI3;
    } else if (strstr(modelString, "Raspberry Pi 4")){
      p = PLATFORM_PI4;
    } else {
      p = PLATFORM_OTHER;
    }
  }
}

platform_t platform() {
  return p;
}

const char* platform_name() {
  switch (platform()) {
  case PLATFORM_CM3:
    return "cm3";
  case PLATFORM_CM4:
    return "cm4";
  case PLATFORM_CM4S:
    return "cm4s";
  case PLATFORM_PI3:
    return "pi3";
  case PLATFORM_PI4:
    return "pi4";
  case PLATFORM_OTHER:
    return "other";
  default:
    break;
  }

  return "unknown";
}