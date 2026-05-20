#include "msg_queue.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
//---- types -------------------------------------------------------------------

struct msg_entry {
    int page_id;
    char *str;
    struct msg_entry *next;
};

//------------------------------------------------------------------------------
//---- variables ---------------------------------------------------------------

static struct msg_entry *head;
static struct msg_entry *tail;
static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

//------------------------------------------------------------------------------
//---- function definitions ----------------------------------------------------

void msg_queue_push(int page_id, const char *str) {
    struct msg_entry *entry = malloc(sizeof(*entry));
    if (!entry) {
        return;
    }
    entry->str = strdup(str);
    if (!entry->str) {
        free(entry);
        return;
    }
    entry->page_id = page_id;
    entry->next = NULL;

    // link into the tail under the lock
    pthread_mutex_lock(&queue_lock);
    if (tail) {
        tail->next = entry;
    } else {
        head = entry;
    }
    tail = entry;
    pthread_mutex_unlock(&queue_lock);
}

bool msg_queue_pop(int *page_id, char **str) {
    pthread_mutex_lock(&queue_lock);

    struct msg_entry *entry = head;
    if (!entry) {
        pthread_mutex_unlock(&queue_lock);
        return false;
    }

    head = entry->next;
    if (!head) {
        tail = NULL;
    }

    pthread_mutex_unlock(&queue_lock);

    *page_id = entry->page_id;
    *str = entry->str;
    free(entry);
    return true;
}

void msg_queue_free(void) {
    int page_id;
    char *str;
    while (msg_queue_pop(&page_id, &str)) {
        free(str);
    }
}
