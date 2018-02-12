# norns

quickstart to the screen interface

## start

upon startup norns is in *script mode* (what to call this really), where the keys and knobs are entirely defined by the user script (?) running. **except** a key 1, which is special.

## labeling

keys and encoders are referred to from left to right, starting from 1. so key 1 is in the upper left, 2 is lower left, 3 is lower right. same for the encoders.

## menu

get into the menu with a short press of key 1. long presses are passed to the user script, so they are useful as a mod/alt key (or a function that is more concerned with hold length or release timing).

### HOME

the home screen shows the current user script running at the top. options are:

- select: load a new script
- parameters: value set for current script (TODO)
- system: wifi/etc
- sleep: turn off

### navigation

- key 2: BACK (think, left)
- key 3: FORWARD/CONFIRM/TOGGLE/ACTIVATE (or, right)

- enc 1: menu global: output level (volume, ie, panic volume if user script didn't add control)
- enc 2: scroll cursor up/down
- enc 3: change value (not used often, see system/gains, will be used in params)

HOME is not the leftmost screen. go LEFT from HOME and you get to STATUS.

### STATUS

system vu's

TODO: direct-to-disk record (tape), monitor control

the idea of having STATUS be leftmost is for quick/easy navigation to this important screen (get in the menu, hit key 2 a bunch of times)

to toggle out of menu mode, hit key 1 quickly again.

the menu remembers which screen it was previously on! handy for flipping back for a glimpse.

### SELECT

go right with key 3, into select.

you'll get a folder/file list. you can dig into subfolders and then go back.

key 3 on a file to load it.

(TODO: key 3 on a file will go into PREVIEW mode which will display some meta-data luadoc-style about the file you're about to load, whereupon you can then hit key 3 again to load. function like a confirmation and whatisit, or you can back out of course.)

### PARAMETERS

TODO. basic list a params like a default view VST, list of params and sliders. or like the aleph. need to design a pattern that uses an expected table that has a list of names and functions. maybe ranges for a simple default setter/getter function. but a custom function could do more complex things, ie, store a preset-- which could traverse the rest of the param table. basic scaffolding to make certain types of scripts easy/fast to make without requiring you to have to edit/reload the script just to change values, or make your own gui interactions.

### SYSTEM

shows battery percentage and power draw at the top (should this move?)

- wifi: shows ip and signal strength. FORWARD to get into wifi menu
- input gain: key 3 to toggle which level is selected, then enc 3 to change (pretty cool!)
- headphone: enc 3 to change
- log: norns version and key 3 to get into the log

#### wifi

off/router/hotspot

router ssid/password set elsewhere, probably via maiden (norns-web)

#### log

enc 2 to scroll. contains both system messages and whatever things the user script wants to print.

### SLEEP

key 3 to confirm and shut down!
