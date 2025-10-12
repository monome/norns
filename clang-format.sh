#!/bin/bash
# run this from top-level norns directory before committing...
find ws-wrapper/src -regex .*[\.][ch] | xargs clang-format -i
find matron/src -regex .*[\.][ch] | xargs clang-format -i
find maiden-repl/src -regex .*[\.][ch] | xargs clang-format -i
find crone/src -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' \) | xargs clang-format -i
find watcher/src -regex .*[\.][ch] | xargs clang-format -i
find tests -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) | xargs clang-format -i
