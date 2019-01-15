#!/bin/bash

# do not try reserving device (disable dbus)
export JACK_NO_AUDIO_RESERVATION=1

# start jack clients
# scsynth -u 57122 -i 2 -o 2 &
./build/crone/crone &

# start sclang (starts local scsynth, performs connections)
SCLANG=$(which sclang)
./build/ws-wrapper/ws-wrapper ws://*:5556 $SCLANG &
