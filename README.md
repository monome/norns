## requirements

lua 

`pacman -S lua`

supercollider

`pacman -S supercollider`


libmonome

[https://github.com/monome/libmonome]


## use

build and run:
`cd maiden`
`make`
`./maiden`

this will open grid as serial device (currently hardcoded to `/dev/ttyUSB0`) and run lua. 

user code handlers are defined in `maiden/handle.lua`

`maiden/init.lua` is executed on launch - extend it to define vars, tables, functions &c for use in handlers.


