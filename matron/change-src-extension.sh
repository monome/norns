#!/bin/bash

shopt -s globstar

for file in **/*.$1; do
    base=$(dirname "$file")/$(basename "$file" .$1)
    mv $base.$1 $base.$2
done
