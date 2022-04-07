/*
 * gpio.c
 *
 * keys and encoders
 */

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "events.h"
#include "hardware/input/matron_input.h"
#include "hardware/gpio.h"

static int input_init(matron_input_t* input, input_config_t* cfg, input_ops_t* ops) {
    input->data = malloc(ops->data_size);
    if (!input->data) {
        fprintf(stderr, "ERROR (input - %s) cannot allocate memory\n", ops->name);
        return -1;
    }
    input->config = malloc(sizeof(input_config_t));
    if (!input->config) {
        fprintf(stderr, "ERROR (input - %s) cannot allocate memory\n", ops->name);
        return -1;
    }
    memcpy(input->config, cfg, sizeof(input_config_t));
    input->ops = ops;
    return 0;
}

// extern def
TAILQ_HEAD(tailhead, _matron_input) input_devs = TAILQ_HEAD_INITIALIZER(input_devs);

int input_create(input_type_t type, const char* name, input_config_t* cfg) {
    int err;
    matron_input_t* input = malloc(sizeof(matron_input_t));
    input->name = malloc(strlen(name) + 1);
    strcpy(input->name, name);

    (void)cfg;

    switch (type) {
        case INPUT_TYPE_GPIO_KEY:
            if ((err = input_init(input, cfg, &gpio_key_ops))) goto fail;
            break;
        case INPUT_TYPE_GPIO_ENC:
            if ((err = input_init(input, cfg, &gpio_enc_ops))) goto fail;
            break;
        /* case INPUT_TYPE_KBM: */
        /*     if ((err = input_init(input, &kbm_input_ops))) goto fail; */
        /*     break; */
        /* case INPUT_TYPE_JSON: */
        /*     if ((err = input_init(input, &json_input_ops))) goto fail; */
        /*     break; */
        default:
            goto fail;
    }

    TAILQ_INSERT_TAIL(&input_devs, input, entries);
    fprintf(stderr, "added input %s\n", name);
    return 0;

fail:
    fprintf(stderr, "couldn't set up input %s\n", name);
    free(input);
    return -1;
} 

void gpio_init() {
    matron_input_t *input;
    TAILQ_FOREACH(input, &input_devs, entries) {
        input->data = malloc(input->ops->data_size);
        if (!input->data) {
            fprintf(stderr, "ERROR (input %s) couldn't allocate %zu\n", input->name, input->ops->data_size);
            return;
        }
        if (input->ops->setup(input)) {
            fprintf(stderr, "ERROR (input %s) setup failed\n", input->name);
            continue;
        }
        if (pthread_create(&input->poll_thread, NULL, input->ops->poll, input)) {
            fprintf(stderr, "ERROR (input %s) pthread error\n", input->name);
            continue;
        }
    }
}

void gpio_deinit() {
    matron_input_t* input;
    while (!TAILQ_EMPTY(&input_devs)) {
        input = TAILQ_FIRST(&input_devs);
        pthread_cancel(input->poll_thread);
        TAILQ_REMOVE(&input_devs, input, entries);
        free(input->data);
        free(input);
    }
}

void *key_check(void *x) {
    (void)x;
    int rd;
    unsigned int i;
    struct input_event event[64];

    while (1) {
        rd = read(key_fd, event, sizeof(struct input_event) * 64);
        if (rd < (int)sizeof(struct input_event)) {
            fprintf(stderr, "ERROR (key) read error\n");
        }

        for (i = 0; i < rd / sizeof(struct input_event); i++) {
            if (event[i].type) { // make sure it's not EV_SYN == 0
                // fprintf(stderr, "enc%d = %d\n", n, event[i].value);
                union event_data *ev = event_data_new(EVENT_KEY);
                ev->key.n = event[i].code;
                ev->key.val = event[i].value;
                event_post(ev);
            }
        }
    }
}
