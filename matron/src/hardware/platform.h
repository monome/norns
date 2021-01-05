#pragma once

typedef enum {
  PLATFORM_UNKNOWN = 0,
  PLATFORM_OTHER,
  PLATFORM_CM3,
  PLATFORM_PI3,
} platform_t;

extern void init_platform(void);
extern platform_t platform(void);
