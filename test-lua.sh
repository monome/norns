#!/bin/bash
# norns lua test runner
# discovers and runs all *_test.lua files in lua/test/ subdirectories

echo -e "\033[1mrunning norns lua tests\033[0m"
echo "-"

# set lua path to include norns modules and testing framework
export LUA_PATH="?.lua;?/init.lua;lua/?.lua;lua/lib/?.lua;lua/test/?.lua"

# run the test runner - this will discover and run all *_test.lua files
lua5.3 lua/test/test_runner.lua