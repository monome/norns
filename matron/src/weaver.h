#pragma once

// initialize the lua VM and run setup scripts
extern void w_init(void);

// compile and execute an arbitrary chunk of lua 
extern void w_run_code(const char* code);
