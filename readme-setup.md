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

the `crone` audio engine consists of supercollider classes. cd to `sc` folder and run `install.sh` to copy these to the default location for user SC extensions. 

see  [readme-usage.md](readme-usage.md) for instructions on running and using norns.

## configure

- add `/usr/local/lib` to library serach paths (if libmonome is installed from sources.)
the recommended way to do this is by editing `/etc/ld.so.conf`. (use of the `LD_LIBRARY_PATH` variable is deprecated, since it willl override binary-specific settings.)

- add udev rules. matron uses `libudev` and `libevdev` for low-level access to input devices. (TODO: see (http://www.reactivated.net/writing-udev-rules.html) ... )

_**TODO: additional setup steps for raspberry pi?**_
