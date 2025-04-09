#!/bin/bash

# List of directories with Makefiles
DIRS=("shared" "packet_hunter" "daemon" "kernel_module")

echo "Starting full build..."

# Move to the root of the project
cd "$(dirname "$0")/.." || exit 1

for dir in "${DIRS[@]}"; do
    echo "Building $dir..."
    if make -C "$dir"; then
        echo "Built $dir"
    else
        echo "Build failed for $dir"
        exit 1
    fi
done

echo "All components built successfully."
