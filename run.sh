#!/bin/bash

INPUT_DIR="input"
OUTPUT_DIR="our_output"
OUTPUT_SUFFIX="_output.txt"
OS_BIN="./os"

mkdir -p "$OUTPUT_DIR"

for config in "$INPUT_DIR"/*; do
    if [[ -d "$config" ]] || [[ "$config" == "$INPUT_DIR/proc"* ]]; then
        continue
    fi

    config_name=$(basename "$config")
    output_file="${OUTPUT_DIR}/${config_name}${OUTPUT_SUFFIX}"

    echo "Running $OS_BIN with config $config..."
    $OS_BIN "$config_name" > "$output_file" 2>&1
done
