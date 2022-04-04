#!/bin/sh
set -e
cd "$(dirname "$0")"
git pull
git submodule update
./waf configure --release
./waf -j2
