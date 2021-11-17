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

#include "hardware/input.h"
#include "hardware/io.h"

int input_setup(matron_io_t *io) {
    if (io->ops->type != IO_INPUT) {
        fprintf(stderr, "ERROR (%s) wrong IO type\n", io->ops->name);
        return -1;
    }

    int err = 0;
    matron_input_t *input = (matron_input_t *)io;
    input_ops_t *input_ops = (input_ops_t *)io->ops;
    err = pthread_create(&input->poll_thread, NULL, input_ops->poll, input);
    if (err) {
        fprintf(stderr, "ERROR (input %s) pthread error\n", io->ops->name);
        return err;
    }

    return 0;
}

void input_destroy(matron_io_t *io) {
    if (io->ops->type != IO_INPUT) {
        fprintf(stderr, "ERROR (%s) wrong IO type\n", io->ops->name);
        return;
    }
    matron_input_t *input = (matron_input_t *)io;
    pthread_cancel(input->poll_thread);
}
