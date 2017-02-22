# build / install

## requirements

```
liblo
libmonome
lua 
SDL2
supercollider
```

except for `libmonome`, these are all available as arch packages:
`pacman -S lua supercollider liblo sdl2`

## build

run `make` from the top level directory. this should create the executables `norns` and `matron/matron`.

## install supercollider classes

`cd ~/norns/crone`
`./install.sh`

this copies the norns supercollider classes to the default SC extensions folder. nothing else needs to be done.

# usage

### launch supercollider

at the moment, supercollider needs to be launched first. type either `sclang` for a bare interpreter instance, or `scide` for the IDE. either way, all the work for Norns is done during class initialization; no interpreted code need be executed. 

if the classes are installed correctly, you should see some lines like this in output from sclang initialization: 
```
-------------------------------------------------
 Crone startup

 OSC rx port: 57120
 OSC tx port: 8888
--------------------------------------------------
```

and immediately after sclang init, you should see the server being booted and some jack/alsa related messages. 

### launch `norns`

with supercollider running, type `./norns` from the top level directory. this in turn executes "matron" (and other components, in the future.) you should now have a rather bare-bones lua REPL. `lua/norns.lua` will be executed at launch; it defines among other things a user startup function. currently this in turn executes the demo script `lua/sticksine.lua`, which uses the joystick to control a sine wave. (maybe not exactly super inspiring, but it's a start!)

## features

for now, the term i'm using for a custom audio processing unit is *engine*. so far there is only trivial engine, called `TestSine`. 

each engine presents an arbitrary collection of 'commands'. each command consists of a name and zero or more arguments. each argument can be one of three types: `int`, `float`, or `string`. thus, engine commands map directly to a subset of OSC messages.

### timers 

currently, there are a fixed number of high-resolution timers that can be set from lua. there is a single callback function for all timers, which takes the timer index as its first argument. 

### controlling audio from lua

the following lua-C bindings are created on initialization of the LVM, and are always available:

- `report_engines()` : send a request to the audio server to list the names of all available engines. the response will be an array of strings, and is handled by `report.engine()` defined in `norns.lua`.

- `load_engine(name)` : request the audio server to load the named engine

- `report_commands()` : request the current audio engine to list available commands, and populate the `engine` table with command functions (see below)

- `send_command(idx, ...)` : send an indexed command with a variable number of arguments. 

- `start_timer(idx, seconds, count)` : start the timer at the given index with a given period and count of callbacks. the first callback will occur immediately. a count value <0 creates an infinite timer.

- `stop_timer(idx)` : stop the timer at the given index.

additionally, engine command functions are created dynamically on reception of a command list from the audio server. these are placed in a table called `engine`, which als ohas the shortcut `e`. so for example, the `TestSine` engine reports just two commands, each of which takes a single float argument:
```
1: hz (f)
2: amp (f)
```

on receiving this report, norns creates two functions whose definitions would look like this:
```
engine.hz = function(arg1) 
  send_command(0, arg1)
end
```

the user then can `engine.hz(440)` or `e.hz(440)` to issue a pitch change command.

### responding to input in lua

input event handlers are defined in `norns.lua`. the default functions just print values; override them in user scripts to define new functionality.


# next work / current issues

- `norns` launcher doesn't clean up its children properly, and it doesn't launch sclang

- make some useful engines in supercollider (first up is a sample cutter)

- test monome grid events with hardware

- make some signalling for *matron* to wait for the SC server to boot

- add some dynamic control to timers (reset, change period, &c) (requires thread sync)

- add a channel for matron to request values from crone. 'requests' to complement 'commands'...

- start screen mockup

- start ncurses client interface
