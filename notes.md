nov 30 2016

# norns


## super condensed software spec

- scriptable sound engine, ie lua controlling scsynth
- REPL with framework (function stubs) and bindings for OSC, script loading, etc
- scripts are basically:
	- init function (set up data, timers, etc)
	- callbacks (timers, encoders/keys, grids/midi, synth events, etc)
	- preset system (state saving/recall) w/ file management
	- each function with commands to get/set synth parameters
- syntax evaluation and soft failure



---

## hardware description

- embedded linux system (rpi3 compute likely)
- audio codec, stereo in/out
	audio jacks, 1/4" mono jacks
	headphone driver on 1/4" stereo jack with volume knob
- usb host port(s) usb whatever (midi, grids, etc)
- powered via mini-usb
	ethernet-over-usb on data lines (for when connected to pc)
	small lithium-ion battery internal
		timed switch to suspend, fast power-ups
- wifi (?)
- i2c jack(s)
- low resolution OLED
- user controls - 2 keys, 2 low res encoders
- sys controls - 1 key (mode), 1 low res encoder (assignable audio volume)

## software

- scriptable sound engine
	ie, lua controlling scsynth

- potential components, separate or combined applications:
	- actual sound engine (scsynth or otherwise)
	- lua engine / loader / REPL
	- low level libraries or application (via OSC/etc)
		communicate with screen, collect encoder data/etc
	- system-interface application
		manage script loading, coordinate interfacing of UI, etc

- "scripts" (scenes) loaded from a list (folder/etc) via hardware user sys-interface (oled/keys/encoders)

- sys-interface needs access to:
	current VU levels for audio i/o
	soft gain stages for audio i/o
	various parameter data ie script name, preset number, etc

## control engine (scripting)

### init

code called at script launch

- initialize variables (ie, feedback level, playback speed, matrix route)
- define additional functions (timers, analysis)
- set up presets (which vars are stored)

### callbacks

functions associated with

- keys
- knobs
- midi
- grids/etc
- i2c
- timers

### presets

state save and recall w/ file management


## sound engine

multiple (arbitrary number?) buffer players, with dynamic buffer assignment

read/write parameter data:

- position
- fade time (on position cut)
- clip (buffer) number
- speed
- loop active/start/end
- loop fade (alignment?)

- playthru level
- record level
- record pre-attenuate (feedback)

- playhead mute
- playhead level

clip funcs:
	- clip length
	- clear
	- destructive trim to loop
	- multiply loop (copy self x number times)
	- disk read/write

matrix routes (?)
	- playhead to output
	- playhead to rechead
	- input to rechead
	- input to output

(slews?)


## development

script reloading
	file change awareness, provided live editing

error reporting
	syntax eval for scripts (lua dump is probably fine)
