#pragma once

extern int args_parse(int argc, char **argv);

extern const char *args_local_port(void);
extern const char *args_remote_port(void);
extern const char *args_scsynth_port(void);
extern const char *args_monome_path(void); // TODO: appears unused, remove? / jah
