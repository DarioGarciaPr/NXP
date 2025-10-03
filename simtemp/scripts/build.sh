#!/bin/bash
set -e

KDIR="/lib/modules/$(uname -r)/build"

echo "[*] Building kernel module..."
make -C ../kernel KDIR=$KDIR

echo "[*] Building user CLI..."
g++ -Wall -O2 -o ../user/cli/simtemp_cli ../user/cli/main.cpp

echo "[*] Build complete"

