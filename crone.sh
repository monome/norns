#!/bin/bash

# do not try reserving device (disable dbus)
export JACK_NO_AUDIO_RESERVATION=1

# ipc wrapper requires full path 
SCLANG=$(which sclang)
./ipc-wrapper/ipc-wrapper $SCLANG ipc:///tmp/crone_in.ipc ipc:///tmp/crone_out.ipc
