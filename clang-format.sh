#!/bin/bash
# run this from top-level norns directory before committing...

FORMAT_EXTENSIONS=( -name '*.c' -o -name '*.h' -o -name '*.cc' -o -name '*.cpp' -o -name '*.hpp' )

find ws-wrapper/src   -type f \( "${FORMAT_EXTENSIONS[@]}" \) | xargs clang-format -i
find matron/src       -type f \( "${FORMAT_EXTENSIONS[@]}" \) | xargs clang-format -i
find maiden-repl/src  -type f \( "${FORMAT_EXTENSIONS[@]}" \) | xargs clang-format -i
find crone/src        -type f \( "${FORMAT_EXTENSIONS[@]}" \) | xargs clang-format -i
find norns            -type f \( "${FORMAT_EXTENSIONS[@]}" \) | xargs clang-format -i
find watcher/src      -type f \( "${FORMAT_EXTENSIONS[@]}" \) | xargs clang-format -i
find osc-test         -type f \( "${FORMAT_EXTENSIONS[@]}" \) | xargs clang-format -i
find tests            -type f \( "${FORMAT_EXTENSIONS[@]}" \) | xargs clang-format -i
