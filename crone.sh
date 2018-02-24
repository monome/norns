#!/bin/bash

# do not try reserving device (disable dbus)
export JACK_NO_AUDIO_RESERVATION=1

# socket wrapper requires full path 
SCLANG=$(which sclang)

./ws-wrapper/ws-wrapper $SCLANG ws://*:5556