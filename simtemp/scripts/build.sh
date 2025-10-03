#!/bin/bash
set -e

# Navegar al directorio del script
cd "$(dirname "$0")"

echo "=== Compilando módulo kernel ==="
cd ../kernel
make clean || true
make
cd ..

echo "=== Compilando CLI (C++) ==="
CLI_DIR="user/cli"
KERNEL_DIR="kernel"

g++ -std=c++17 -Wall -Wextra -I"$KERNEL_DIR" -o "$CLI_DIR/main" "$CLI_DIR/main.cpp"

echo "=== Compilación completada ==="

