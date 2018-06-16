#!/bin/bash

# do not try reserving device (disable dbus)
export JACK_NO_AUDIO_RESERVATION=1

SCLANG=$(which sclang)
./build/ws-wrapper/ws-wrapper ws://*:5556 $SCLANG
