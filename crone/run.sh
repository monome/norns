#!/bin/bash

#FIXME: read port config from somewhere?
scsynth -u 57121 -V -2
& sclang -u 57120 main.scd
