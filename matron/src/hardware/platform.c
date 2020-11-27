#include <unistd.h>
#include <stdlib.h>

#include "platform.h"

platform_t p;

void init_platform() {
  if(access("/sys/firmware/devicetree/base/model", F_OK ) != -1) {
    if(system("cat /sys/firmware/devicetree/base/model | grep 'Compute'")) {
      p = PLATFORM_PI3;
    } else {
      p = PLATFORM_CM3;
    }
  } else {
    p = PLATFORM_OTHER;
  }
}

platform_t platform() {
  return p;
}
