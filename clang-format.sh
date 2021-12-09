#!/bin/bash
# run this from top-level norns directory before committing...
find ws-wrapper/src -regex .*[\.][ch] | xargs clang-format -i
find lachesis/src -regex .*[\.][ch] | xargs clang-format -i
