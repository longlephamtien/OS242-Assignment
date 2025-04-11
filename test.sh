#!/bin/bash

mkdir -p our_output

for input in src/input/*; do
    filename=$(basename "$input")
    ./os "$input" > "our_output/$filename"
    ./test "$filename"
done
