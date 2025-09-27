#!/bin/bash
set -e
echo "Building kernel module..."
cd ../kernel
make
echo "Kernel module built."

echo "Building CLI C++..."
cd ../user/cli
make
echo "CLI built."
