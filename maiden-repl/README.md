maiden-repl
===========

the `maiden-repl` command is a terminal based alternative to the `matron`
and `supercollider` (sc/crone) REPLs provided in the web based `maiden` editor. this
command can be used independently of, or concurrently with, the web based editor. any
REPL output will appear in all connected sessions but REPL input commands and command
history.

usage
-----

`maiden-repl <matron_socket> <crone_socket>`

the default values for `matron_socket` and `crone_socket` are
`ws://localhost:5555/` and `ws://localhost:5556/` respectively which are
appropriate if `maiden-repl` is being run directly on `norns` (via ssh or serial
connection).

if running `maiden-repl` from another host insert the appropriate host name (or
IP address) for
your device such as:

```
maiden-repl ws://norns.local:5555 ws://norns.local:5556
```

building
--------

`maiden-repl` is known to build on:
* monome norns, norns shield os images
* linux (debian buster)
* macOS (10.14 Mojave)

when building directly on a norns device `maiden-repl` should be built by
default as part of the `waf` based build system for the full norns software
stack (matron, crone, etc.)

when building on linux or macOS the a secondary `cmake` based build sytem allows
for building the command independently of the full norns stack. the `cmake`
system is very rudimentary.

on linux manually install the following dependencies:
```
sudo apt-get install cmake libncursesw5-dev libreadline-dev libnanomsg-dev
```

on macOS ensure the XCode developer commandline tools are installed then manually install the following (via Homebrew):
```
brew install cmake readline nanomsg
```

from there on either linux or macOS:
```
git clone https://github.com/monome/norns.git
cd norns/maiden-repl
mkdir build
cd build
cmake ..
make
```

the resulting `maiden-repl` command in the build directory can be copied out and
placed in a directory which is on one's `PATH`.

