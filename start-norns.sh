#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -e ~/norns ]; then
    ln -s "$SCRIPT_DIR" ~/norns
fi

# Create dust directories if missing
if [ ! -e ~/dust ]; then
    mkdir -p ~/dust/code ~/dust/data ~/dust/audio
fi

# Install SC config (adds include paths for ~/norns/sc/core, ~/norns/sc/engines, ~/dust)
SC_EXT=~/Library/Application\ Support/SuperCollider/Extensions
if [ ! -e "$SC_EXT/norns-config.sc" ]; then
    mkdir -p "$SC_EXT"
    cp "$SCRIPT_DIR/sc/norns-config.sc" "$SC_EXT/"
fi

./stop-norns.sh

DEVICE="${1:-~:AMS2_Aggregate:0}"

# Start jackd
jackd -d coreaudio --device "$DEVICE" &
JACK_PID=$!
sleep 2

# Start crone
./build/crone/crone &
CRONE_PID=$!
sleep 1

# Start SuperCollider (sclang starts scsynth internally)
/Applications/SuperCollider.app/Contents/MacOS/sclang -D &
SC_PID=$!
sleep 3

# Start matron (foreground)
./build/matron/matron

# Cleanup on exit
kill $CRONE_PID $SC_PID $JACK_PID 2>/dev/null
