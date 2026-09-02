#!/bin/bash
set -e

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

echo "Build successful! Running regression tests..."
cd build
ctest --output-on-failure
