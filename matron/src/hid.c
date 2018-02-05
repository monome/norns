#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>

#include "events.h"
#include "hid.h"

static pthread_t pid;

#define RX_BUF_LEN 4096
static void *hid_run(void *p) {
    (void)p;
    bool quit = false;
    char rxbuf[RX_BUF_LEN];
    int nb;
    bool newline;
    char b;

    while(!quit) {
        nb = 0;
        newline = false;
        while(!newline) {
            if(nb < RX_BUF_LEN) {
                read(STDIN_FILENO, &b, 1);
                if(b == '\0') { continue; }
                if(b == '\n' || b == '\r') { newline = true; }
                rxbuf[nb++] = b;
            }
        }
        if(nb == 2) {
            if(rxbuf[0] == 'q') {
                event_post( event_data_new(EVENT_QUIT) );
                fflush(stdout);
                quit = true;
                continue;
            }
        }
        if (nb > 0) {
            // event handler must free this chunk!
            char *line = malloc( (nb + 1) * sizeof(char) );
            strncpy(line, rxbuf, nb);
            line[nb] = '\0';
            union event_data *ev = event_data_new(EVENT_EXEC_CODE_LINE);
            ev->exec_code_line.line = line;
            event_post(ev);
        }
    }
    return NULL;
}

void hid_init(void) {
    pthread_attr_t attr;
    int s;
    s = pthread_attr_init(&attr);
    if(s != 0) { printf("hid_init(): error in pthread_attr_init(): %d\n", s); }
    s = pthread_create(&pid, &attr, &hid_run, NULL);
    if(s != 0) { printf("hid_init(): error in pthread_create(): %d\n", s); }
    pthread_attr_destroy(&attr);
}
