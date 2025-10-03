#!/bin/bash
set -e

echo "=== Compilando kernel module ==="
make -C /lib/modules/$(uname -r)/build M=$(pwd)/kernel modules

echo "=== Compilando CLI C++ ==="
g++ -std=c++17 -O2 -o user/cli/simtemp_cli user/cli/main.cpp

echo "=== Build completado ==="

