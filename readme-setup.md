# norns setup

## build prerequisites

### standard packages

these can be installed from the default Debian repositories: 

```
sudo apt-get install libevdev-dev liblo-dev libudev-dev libcairo2-dev liblua5.3-dev libavahi-compat-libdnssd-dev libasound2-dev libncurses5-dev libncursesw5-dev libsndfile1-dev libjack-dev libnanomsg-dev
```

### other packages / sources

on norns / raspberry pi, the following should be installed from monome's repositories:

```
curl https://keybase.io/artfwo/pgp_keys.asc | sudo apt-key add -
echo "deb https://package.monome.org/ stretch main" | sudo tee /etc/apt/sources.list.d/norns.list
sudo apt update
sudo apt install libmonome-dev libnanomsg-dev supercollider-language supercollider-server supercollider-dev
```

for other platforms (x86, amd64), these packages can again use the standard repositories:

```
sudo apt install libnanomsg-dev supercollider-language supercollider-server supercollider-dev
```

and `libmonome` must be built and installed from source:

- clone, build, and install:
```
git clone https://github.com/monome/libmonome
cd libmonome
./waf configure && ./waf && sudo ./waf install
```

- add `/usr/local/lib` to library search paths. the recommended way to do this is by editing `/etc/ld.so.conf`. (use of the `LD_LIBRARY_PATH` variable is deprecated, since it willl override binary-specific settings.)


### desktop

using the `desktop` build option has additional requirements:
```
sudo apt install libsdl2-dev
```

## building norns

```
git clone https://github.com/monome/norns.git
cd norns
git submodule update --init --recursive
./waf configure
./waf
```

this should build several executables under `norns/build/<name>/`:

- `matron`: the main norns system application: runs scripts, interfaces with controllers and screens
- `crone`: the norns "audio backend," a JACK application which routes audio, runs the `softcut` buffer processing system, and hosts built-in effects
- `ws-wrapper`: utility which wraps the standard I/O of a process in websockets; used to expose `matron` and `sclang` to the web IDE
- `watcher`: watchdog utility
- `maiden-repl`: command-line interface to `matron` and `sclang` REPLs

(note that the `maiden` webserver/IDE is managed separately, in its own repository.)

### supercollider 
norns also uses some custom supercollider classes. thess files must be copied to the default location for user SC extensions; we provide a script to do so:

```
pushd sc
./install.sh
popd
```

### desktop

for building on desktop, add the `--desktop` option to both `waf` steps (configure and build):

```
./waf configure --desktop
./waf build --desktop
```

(NB: `waf` assumes `build` as the default command, which is why we can omit it above.)


## launching components

in normal use, the component executables should be managed by `systemd` services, which take care of websocket wrapping.

for development, it is often useful to run component processes manually from an `ssh` session. this is best accomplished using three different sessions (`screen` is an option), each launching the following in this order:

- 0. first, ensure that JACK is running: `systemctl start norns-jack`
- 1. `norns/build/crone/crone` - runs the norns audio backend; prints some status stuff, but is not interactive
- 2. `sclang` - runs supercollider, spawns a `scynth` instance which is then routed through `crone`; provides a nice REPL
- 3. `norns/build/matron/matron` - runs the main norns system; provides a very basic REPL (no readline)

in general, changes to `matron` sources require only building and relaunching `matron`, changes to supercollider classes require relaunching `sclang`+`matron`, and changes to `crone` sources require relaunching all three.

## launching `maiden` (web UI client)

get most [recent version](https://github.com/monome/maiden/releases)

download to `~/maiden/` and untar

execute with `./maiden.arm -debug -site ./app/build -data ~/norns/lua/`

## launching on desktop / other platforms:

when running norns on desktop computers or custom hardware platforms, you will want to provide `matron` with appropriate runtime configuration options using the `matronrc.lua` file. this should be copied to the user's home directory and customized there. see the comments in that file.

## docs

if you want to generate the docs (using ldoc) first install:

```
sudo apt-get install luarocks liblua5.1-dev
sudo luarocks install ldoc
```

to generate the docs:

`ldoc .` in the root norns folder

To read the documentation, point the browser window with Maiden loaded to [http://norns.local/doc](http://norns.local/doc) (or use IP address if this doesn't work).

