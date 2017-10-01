#!/bin/bash

# ipc wrapper requires full path 
SCLANG=$(which sclang)
SCSYNTH=$(which scsynth)

# fixme: might want to redirect scsynth output
$SCSYNTH -u 57110 &
./ipc-wrapper/ipc-wrapper $SCLANG ipc:///tmp/crone_in.ipc ipc:///tmp/crone_out.ipc
