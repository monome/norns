#!/bin/bash
git pull
git submodule update
./waf configure
./waf

