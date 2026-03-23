#pragma once

/*
 * thread-safe message queue.
 */

#include <stdbool.h>

// enqueue a line for a given page (thread-safe, copies str)
extern void msg_queue_push(int page_id, const char *str);

// dequeue the next line
// returns false when empty
// caller must free(*str) after use
extern bool msg_queue_pop(int *page_id, char **str);

// drain and free all remaining entries
extern void msg_queue_free(void);
