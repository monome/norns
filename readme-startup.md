### norns startup process

a quick outline describing how `matron` starts

1. after some initialization matron will wait for a *ready* OSC message from crone (the dsp, ie supercollider), pinging crone with `o_ready()`
2. once received, `w_init()` (weaver.c) first executes `config.lua` to get paths set up
3. then `norns.lua` is loaded via a require
4. `norns.lua` sets up a bunch of definitions that will be redefined later
5. matron resumes initializing and then runs `w_startup()` which calls the *startup* function defined earlier in `norns.lua`-- which runs `startup.lua`
6. `startup.lua` loads all of the other modules, some shortcuts, and then calls `norns.state.resume()` which loads the last running script (which is stored in `state.lua`)
