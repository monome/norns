#!/bin/bash

# do not try reserving device (disable dbus)
export JACK_NO_AUDIO_RESERVATION=1

# ipc wrapper requires full path 
SCLANG=$(which sclang)
./build/ws-wrapper/ws-wrapper $SCLANG ws://*:5556
