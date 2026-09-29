#!/bin/sh
set -e
cd "$(dirname "$0")"
git pull
git submodule update --init --recursive
./waf configure --release
./waf build --release -j2
