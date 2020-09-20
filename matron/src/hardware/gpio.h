#pragma once

#include <stdint.h>
#include <sys/queue.h>

typedef enum _input_type {
    INPUT_TYPE_GPIO_KEY,
    INPUT_TYPE_GPIO_ENC,
    INPUT_TYPE_KBM,
    INPUT_TYPE_JSON,
} input_type_t;

typedef struct _input_config {
    int index;
    char* dev;
} input_config_t;

extern int input_create(input_type_t type, const char* name, input_config_t* cfg);
extern void gpio_init(void);
extern void gpio_deinit(void);
