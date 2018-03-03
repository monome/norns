# norns setup

## build prerequisites

### packages
```
sudo apt-get install libevdev-dev liblo-dev libudev-dev libcairo2-dev liblua5.3-dev
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
./waf configure
./waf
```

this should build all the c-based components (`matron` and `ws-wrapper`.)

the `crone` audio engine consists of supercollider classes. run `sc/install.sh` to copy these to the default location for user SC extensions. 


## configure

- add `/usr/local/lib` to library serach paths (if libmonome is installed from sources.)
the recommended way to do this is by editing `/etc/ld.so.conf`. (use of the `LD_LIBRARY_PATH` variable is deprecated, since it willl override binary-specific settings.)


## launching components

### 1. launch `crone` (audio engine)

run `crone.sh` from the norns directory. this creates a `sclang` process wrapped with `ws-wrapper`

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

with the audio engine running, run `matron.sh` from the norns directory. this creates a `matron` process wrapped with `ws-wrapper`

matron waits for crone to finish loading before entering the main event loop.

### 3. launch `maiden` (web UI client)

TODO

ie. `./norns-web.arm -debug -site ./app/build -data ~/norns/lua`
