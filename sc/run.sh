#!/bin/bash

# fixme: would like to have separate access to scsynth's stdio,
# so we can use the normal terminal behavior to trigger library recompile.
# haven't figured it out yet.

scsynth -u 57121 -V -2
& sclang -u 57120 main.scd
