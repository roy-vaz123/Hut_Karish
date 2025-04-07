#!/bin/bash

# List of directories with Makefiles
DIRS=("shared" "packet_hunter" "daemon")

echo "Starting full clean and build..."

# Move to the root of the project
cd "$(dirname "$0")/.." || exit 1

for dir in "${DIRS[@]}"; do
    echo "Cleaning $dir..."
    if make -C "$dir" clean; then
        echo "Cleaned $dir"
    else
        echo "Failed to clean $dir"
    fi

    echo "Building $dir..."
    if make -C "$dir"; then
        echo "Built $dir"
    else
        echo "Build failed for $dir"
        exit 1
    fi
done

echo "All components built successfully."
