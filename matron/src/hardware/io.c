#include <stdlib.h>
#include <sys/queue.h>

#include <lauxlib.h>

#include "hardware/io.h"
#include "hardware/screen/screens.h"
#include "hardware/input/inputs.h"

io_ops_t* io_types[] = {
    (io_ops_t*)&enc_gpio_ops,
    (io_ops_t*)&key_gpio_ops,

#ifdef NORNS_DESKTOP
    (io_ops_t*)&screen_sdl_ops,
    (io_ops_t*)&input_sdl_ops,
#endif
    (io_ops_t*)NULL,
};

struct io_head io_queue = TAILQ_HEAD_INITIALIZER(io_queue);

int io_create(lua_State *l, io_ops_t *ops) {
    matron_io_t* io;
    switch (ops->type) {
        case IO_SCREEN:
            io = malloc(sizeof(matron_fb_t));
            break;
        case IO_INPUT:
            io = malloc(sizeof(matron_input_t));
            break;
        default:
            fprintf(stderr, "ERROR (%s) no such IO type\n", ops->name);
            return -1;
    }
    if (!io) {
        luaL_error(l, "out of memory");
        return -1;
    }

    if (!(io->data = malloc(ops->data_size))) {
        free(io);
        return luaL_error(l, "out of memory");
    }

    io->ops = ops;
    int err = io->ops->config(io, l);
    if (err) {
        free(io->data);
        return err;
    }

    TAILQ_INSERT_TAIL(&io_queue, io, entries);
    return 0;
}

int io_setup_all(void) {
    int err;
    matron_io_t* io;
    TAILQ_FOREACH(io, &io_queue, entries) {
        fprintf(stderr, "setup IO %s\n", io->ops->name); 
        err = io->ops->setup(io);
        if (err) {
            fprintf(stderr, "ERROR (%s) setup failed\n", io->ops->name);
            return err;
        }
    }
    fprintf(stderr, "IO setup OK.\n");
    return 0;
}

void io_destroy_all(void) {
    matron_io_t* io;
    while (!TAILQ_EMPTY(&io_queue)) {
        io = TAILQ_FIRST(&io_queue);
        io->ops->destroy(io);
        TAILQ_REMOVE(&io_queue, io, entries);
        free(io->data);
        free(io);
    }
}
