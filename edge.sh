#!/bin/sh
set -e
cd "$(dirname "$0")"
git pull
git submodule update
./waf configure
./waf -j2
