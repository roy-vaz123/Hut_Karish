#!/bin/bash

# List of directories with Makefiles
DIRS=("shared" "packet_hunter" "daemon" "kernel_module")

echo "Starting full clean..."

# Move to the root of the project
cd "$(dirname "$0")/.." || exit 1

for dir in "${DIRS[@]}"; do
    echo "Cleaning $dir..."
    if make -C "$dir" clean; then
        echo "Cleaned $dir"
    else
        echo "Failed to clean $dir"
    fi
done

echo "All clean steps done."
