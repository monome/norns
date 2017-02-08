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

## build matron

```
cd ~/norns/matron
make
```

this should build the `matron` executable.

## install supercollider classes

`cd ~/norns/crone`
`./install.sh`

this copies the norns supercollider classes to the default SC extensions folder. nothing else needs to be done.

# usage

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

with supercollider running, launch `matron`. this will present a rather bare-bones lua REPL. `matron/lua/norns.lua` will be executed at launch; it defines among other things a user startup function. currently this in turn executes the demo script `matron/lua/sticksine.lua`, which uses the joystick to control a sine wave. (maybe not exactly super inspiring, but it's a start!)

## features

for now, the term i'm using for a custom audio processing unit is *engine*. there is a single engine right now, called `TestSine`. each engine can declare an arbitrary number of audio buffers and control parameters, each accessible by name or by index. each parameter controls a single floating point value, with an arbitrary setter function.

### controlling audio from lua

in lua, several C functions are registered and available for the user. 

- `report_engines()` : sends a request to the audio server to list the names of all available engines. the response will be an array of strings, and is handled by `report.engine()` defined in `norns.lua`.

- `report_params()` : as above, reports the name of all parameters for the current engine, handler is `report.param()`

- `report_buffers()` : as above, reports names of audio buffers for current engine, handler is `report.buffer()`

- `load_engine(name)` : requests the audio server to load the named engine

- `set_param(name, value)` : sets the named parameter to the given value

- `load_buffer(name, path)` : reads the soundfile at the given path into the named buffer.

- `write_buffer(name, path)` : writes the named buffer to a soundfile at the given path.

### responding to input in lua

input event handlers are defined in `norns.lua`. the default functions just print values; override them in user scripts to define new functionality.


# next work / current issues

- lua errors are being handled in a not-very-useful way. i need to check out lua's own REPL implementation to see how to get a stack trace instead of just an error code; until then it is difficult to debug syntax errors in user scripts.

- need to add lua <-> timer glue. the timer system works on its own but haven't tested integration.

- test monome grid events with hardware

- make some useful engines in supercollider (first up is a sample cutter)

- make some signalling for *matron* to wait for the SC server to boot

- wrap sc and matron execution in a launcher app (call it *norns*.) use linux `execl` and `fork`. capture and redirect matron and sclang's standard I/O as desired. 

- might be good to change from the fixed 'buffers and parameters' model, to having each engine declare an arbitrary list of OSC-formatted commands, e.g.:

```
{
	(\path:'/buffer/load', \format: "ss") // load named buffer from soundfile
	(\path;'/buffer/crop', \format: "ff") // crop a buffer to the given start and end locations (in seconds)
	(\path:'/pitch/set/hz', \format: "f") // set pitch as a raw hz value
	(\path:'/waveform/partial/set', \format: "if"// set a given partial to a given amplitude in the waveform
}
```

... and so on.
