#pragma once

#include <sys/queue.h>

#include "hardware/gpio.h"

struct _matron_input;

typedef struct _input_ops {
    size_t data_size;
    const char* name;

    int (*setup)(struct _matron_input* input);
    void* (*poll)(void* input);
} input_ops_t;

typedef struct _matron_input {
    char* name;
    void *data;
    pthread_t poll_thread;
    input_ops_t* ops;
    input_config_t* config;

    TAILQ_ENTRY(_matron_input) entries;
} matron_input_t;

extern input_ops_t gpio_key_ops;
extern input_ops_t gpio_enc_ops;
/* extern input_ops_t kbm_key_ops; */
/* extern input_ops_t kbm_enc_ops; */
