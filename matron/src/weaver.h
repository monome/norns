#pragma once

// initialize the lua VM and run setup scripts
extern void w_init(void);

// run the user-defined startup routine
extern void w_user_startup(void);

// compile and execute an arbitrary chunk of lua
// this blocks execution and access the VM directly;
// call it only from the main thread
extern void w_run_code(const char* code);


//-------------------------
//---- c -> lua glue

//--- hardware input
extern void w_handle_grid_press(int x, int y);
extern void w_handle_grid_lift(int x, int y);

extern void w_handle_stick_axis(int stick, int axis, int value) ;
extern void w_handle_stick_button(int stick, int button, int value) ;

//--- audio engine introspection
extern void w_push_buffer_list(char** arr, int num);
extern void w_push_engine_list(char** arr, int num);
extern void w_push_param_list(char** arr, int num);
