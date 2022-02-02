this is just `jack_cpu_load`, with some trivial modifications to make it more useful for automated profile capturing:

- more useful output format
- specified iteration count
- specified iteration period

usage:

`jack-cpu-capture <n=100> <d=0.25>`