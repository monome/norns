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
libcairo2-dev
liblua5.3-dev
```

for desktop: 
```
supercollider-language
supercollider-server
supercollider-dev
```

### sources

build and install:

```
https://github.com/monome/libmonome.git
https://github.com/nanomsg/nanomsg.git
```

or use the debian repository as follows:

```
curl https://keybase.io/artfwo/pgp_keys.asc | sudo apt-key add -
echo "deb http://norns.catfact.net/ debian/" | sudo tee /etc/apt/sources.list.d/norns.list
sudo apt update
sudo apt install libmonome-dev libnanomsg-dev supercollider-language supercollider-server supercollider-dev
```

## building norns

```
git clone https://github.com/catfact/norns.git
cd norns
git submodule init && git submodule update
./waf configure
./waf
```

this should build all the c-based components (`matron`, `maiden`, and `ipc-wrapper`.)

the `crone` audio engine consists of supercollider classes. run `sc/install.sh` to copy these to the default location for user SC extensions. 

see  [readme-usage.md](readme-usage.md) for instructions on running and using norns.

## configure

- add `/usr/local/lib` to library serach paths (if libmonome is installed from sources.)
the recommended way to do this is by editing `/etc/ld.so.conf`. (use of the `LD_LIBRARY_PATH` variable is deprecated, since it willl override binary-specific settings.)

- add udev rules. matron uses `libudev` and `libevdev` for low-level access to input devices. (TODO: see (http://www.reactivated.net/writing-udev-rules.html) ... )

_**TODO: additional setup steps for raspberry pi?**_



## launching components

### 0. setup environment

run `install.sh` in `norns/crone/` to copy/link required files

### 1. launch `crone` (audio engine)

run `crone.sh` from the norns directory. this creates a `sclang` process wrapped with `ipc-wrapper`, and a pair of ipc sockets in `/tmp`.

if the crone classes are installed correctly, you should see some lines like this in output from sclang initialization: 
```
-------------------------------------------------
 Crone startup

 OSC rx port: 57120
 OSC tx port: 8888
--------------------------------------------------
```

and immediately after sclang init, you should see the server being booted and some jack/alsa related messages. 

### 2. launch `matron` (lua interpreter)

with the audio engine running, run `matron.sh` from the norns directory. this creates a `matron` process wrapped with `ipc-wrapper`, and a pair of ipc sockets in `/tmp`.

### 3. launch `maiden` (UI client)

with the other components running, run the `maiden` executable from anywhere. this presents the user with a REPL interface to both the lua interpreter and the sclang backend.

special characters:

- ~~`TAB` : switch between lua and sclang REPLs~~ this is disabled actually
- `shift+TAB` : switch between output log and input log for each REPL
(_**TODO: input log isn't really implemented**_)
- `q` : quits the client.

_**TODO: output scrolling works, but no commands for it yet.**_

_**TODO: more robust command input, e.g. `ctl-a` as an escape sequence.**_

