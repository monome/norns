# norns setup

## build prerequisites

### packages

install with pacman / apt-get:

```
cmake
libevdev-dev
liblo-dev
libncurses-dev
libncursesw5-dev
libreadline-dev
libudev-dev
```

for desktop: 
```
supercollider-language
supercollider-server
```

for rpi headless (building supercollider): 
```
libcwiid1 libasound2-dev libsamplerate0-dev libsndfile1-dev
```

note that lua packages are *not* required anymore; instead, lua is included as a submoodule, built from source, and statically linked.

### sources

build and install:

```
https://github.com/monome/libmonome.git
https://github.com/nanomsg/nanomsg.git
```

for running on raspberry pi headless, supercollider must be built with some particular twists. 
see here:
http://supercollider.github.io/development/building-raspberrypi.html
and maybe here:
https://github.com/redFrik/supercolliderStandaloneRPI2/blob/master/BUILDING_NOTES.md


## building norns

```
git clone https://github.com/catfact/norns.git
cd norns
git subomdule init && git submodule update
make
```

this should build all the c-based components (`matron`, `maiden`, and `ipc-wrapper`.)

the `crone` audio engine consists of supercollider classes. run `crone/install.sh` to copy these to the default location for user SC extensions. 

see `readme-usage.md` for instructions on running norns.

## configure

- add `/usr/local/lib` to library serach paths (needed by libmonome.)
the recommended way to do this is by editing `/etc/ld.so.conf`. (use of the `LD_LIBRARY_PATH` variable is deprecated, since it willl override binary-specific settings.)

- add udev rules. matron uses `libudev` and `libevdev` for low-level access to input devices. (TODO: see (http://www.reactivated.net/writing-udev-rules.html) ... )

* **TODO: additional setup steps for raspberry pi?** *
