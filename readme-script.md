## lua programming

see `norns/doc` for API reference.

### menu and startup

a menu system manages the execution of a single user script, which is selected through the menu interface. on startup the last-selected user script will be run, and the interface will be in "script" mode (as opposed to menu mode). see Addendum 1 for the details of script startup.

### audio

audio processing is performed by audio *engines*. only one engine is loaded at a time. each engine presents an arbitrary collection of 'commands'. each command consists of a name and zero or more arguments. each argument can be one of three types: `int`, `float`, or `string`. thus, engine commands map directly to a subset of OSC messages.

#### engine control functions:

- `report_engines()` : request a list of available engines

- `load_engine(name)` : request audio server to load the named engine 

- `report_commands()` : request the current audio engine to list available commands, and populate the `engine` table with command functions (see below) 

- `send_command(idx, ...)` : send an indexed command with a variable number of arguments. 

additionally, specific engine command functions are created dynamically on reception of a command list from the audio server, and placed in the table `norns.engine`. so for example, the `TestSine` engine reports just two commands, each of which takes a single float argument:
```
1: hz (f)
2: amp (f)
```

on receiving this report, norns creates two functions whose definitions would look like this:
```
norns.engine.hz = function(arg1) 
  send_command(1, arg1)
end
norns.engine.amp = function(arg1) 
  send_command(2, arg1)
end

```

a shortcut is set on startup: `e = norns.engine`; so the user then can then simply use `e.hz(440)` and `e.amp(0.5)` in this example.

#### engine callbacks:

- `report.engines(names, count)` : called when an engine report is ready. arguments: table of engine names (strings), number of engines.

- `report.commands(commands, count)` : called when a command report is ready. the `commands` argument is a table of tables; each subtable is of the form `{ name, format }`, where `name` is the name of the command and `format` is an OSC-style format string. 

note that commands are reported automatically on engine load. so for the time being, the `report.commands` callback is the easiest method for delaying lua code execution until an engine is finished loading.

- `report.polls(polls, count)` : callback with all "polls" available (current engine plus persistent audio context)

#### high level management

user scripts specify an engine to load with a single line, ie:

`engine = TestSine`

the menu system manages the loading of the engine, and runs the user function `init` once the engine loads.

### Polls

polls provide a way for matron to query crone. for example, getting audio input and output levels for VU display.

TODO

### I/O devices

#### monome 

grid devices can be hotplugged. connected devices are available as tables in `norns.grid.devices` and `norns.arc.devices`. each table includes information about the device as well as methods to control its output.

- `Grid:led(x, y, z)` : set a single led at `(x,y)` to brightness `z`, in the range 0-15.

- `Grid:refresh()` : update the device's physical state.

for device hotplug and gesture input, the following callbacks can be defined:

- `grid.add(device)` : grid device was added. the argument is a `Grid` table, which includes the following fields:
	- `id` : an integer ID. these never repeat during a given run of `matron`.
	- `serial` : a serial string representing the device, like `m1000404`.
	- `name` : a human-readable string describing the device, like `monome 128`.
	- `dev` : an opaque pointer to the raw device handle. this is passed back to C on device update; user scripts shouldn't need to use it.

- `grid.remove(id)` : grid device was removed

- `grid.key(device, x, y, value)` : key event was received on the given device. 

the menu system manages grid functions to simplify user scripts: `gridkey()` is the function which can be redefined in a user script for managing grid input. `gridredraw()` is the grid drawing function.connect/disconnect awareness can likewise be redefined by the user script.


#### HID

HID input devices work similarly to monome devices. however, the event structure is necessarily more complex. 

callbacks:

- `input.add(device) ` : an input device was added. argument is an `InputDevice` table, with the following fields:
    - `id` : an integer ID. these never repeat during a given run of `matron`.
	- `serial` : a serial string representing the device. for now, this is an 8-digit hex string; the first 4 hex digits are the product ID, the last 4 are the vendor ID.
	- `name` : a human-readable string desribing the device (e.g. "Logitech USB Optical Mouse.")
    - `types` : event types supported by this device. 
	- `codes` : a table containing one subtable per supported event type, indexed by event type. each subtable contains one entry per supported event code for that event type; index in subtable is code number, value is code name.
	
	event type and code names are defined in `sys/input_device_codes.lua`. 

- `input.remove()` : _**TODO: not connected yet**_

- `input.event(device, type, code, value)` : respond to an event from the given device.

_**TODO?: HID output**_

#### MIDI

_**TODO**_

### timers

`matron`  maintains a fixed number of high-resolution timers that can be used from lua:

- `timer(index, stage)` : this shared callback function is called whenever any timer fires. arguments are the timer's index and current stage number. *timer index and stage number are 1-based.*

- `timer_start(index, period, count, stage)` : start a timer. the first callback happens immediately. if the timer  is already running, it will be restarted from the given stage.
    - index: 1-based index of the timer.
	- period: seconds between stages. if ommitted, the previous setting for this timer is re-used.
	- count: number of callbacks to perform before stopping. if ommitted, nil, or <=0, the timer will run indefinitely.
	- stage: stage number to start at (1-based.) default is 1.
	
- `timer_stop(index)` : stop the indexed timer immediately. 
		
### graphics

user script screen drawing is managed in the user function `redraw()`. this function is called when the menu enters script mode, and can also be called arbitrarily within the user script (for example, after a key press which changes state somehow).

there are numerous high-level drawing functions, which are a subset of the underlying cairo library (see https://www.cairographics.org). for example:

```
s.clear()
s.move(0,20)
s.text("hello!")
s.update()
```

this clears the screen, moves the current position to (0,20) and then prints "hello!". the final command `s.update()` flips the buffer to the screen, making the prior commands visible.

full drawing reference in the API.





## Addendum 1: Startup Process

a quick outline describing how `matron` starts

1. after some initialization matron will wait for a *ready* OSC message from crone (the dsp, ie supercollider), pinging crone with `o_ready()`
2. once received, `w_init()` (weaver.c) first executes `config.lua` to get paths set up
3. then `norns.lua` is loaded via a require
4. `norns.lua` sets up a bunch of definitions that will be redefined later
5. matron resumes initializing and then runs `w_startup()` which calls the *startup* function defined earlier in `norns.lua`-- which runs `startup.lua`
6. `startup.lua` loads all of the other modules, some shortcuts, and then calls `norns.state.resume()` which loads the last running script (which is stored in `state.lua`)
