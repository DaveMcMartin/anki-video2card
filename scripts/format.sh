#!/bin/bash

# Format all C++ source files in the src directory using clang-format
# This script formats all .hpp, .cpp, .h, and .c files recursively

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$PROJECT_ROOT/src"

if [ ! -d "$SRC_DIR" ]; then
    echo "Error: src directory not found at $SRC_DIR"
    exit 1
fi

echo "Formatting C++ source files in $SRC_DIR"
echo "=========================================="

# Find and format all .hpp, .cpp, .h, and .c files
find "$SRC_DIR" \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -type f | while read -r file; do
    echo "Formatting: $file"
    clang-format -i "$file"
done

echo "=========================================="
echo "Code formatting complete!"
