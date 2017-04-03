#pragma once

#include <stdlib.h>

extern void ui_init(void);
extern void ui_deinit(void);

extern void ui_loop(void);

// receive and display lines from child processes
// lines should be null-terminated strings
extern void ui_crone_line(char *str);
extern void ui_matron_line(char *str);
