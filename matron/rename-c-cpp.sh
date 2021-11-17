#!/bin/bash

shopt -s globstar

for file in **/*.c; do
    base=$(dirname "$file")/$(basename "$file" .c)
    mv $base.c $base.cpp
done
