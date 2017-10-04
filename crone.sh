#!/bin/bash

# ipc wrapper requires full path 
SCLANG=$(which sclang)
./ipc-wrapper/ipc-wrapper $SCLANG ipc:///tmp/crone_in.ipc ipc:///tmp/crone_out.ipc
