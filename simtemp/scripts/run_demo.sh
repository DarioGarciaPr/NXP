#!/bin/bash
set -e

# Determinar la ruta base del proyecto (donde está simtemp/)
BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
KERNEL_DIR="$BASE_DIR/kernel"
CLI_DIR="$BASE_DIR/user/cli"

echo "Base directory: $BASE_DIR"
echo "Kernel directory: $KERNEL_DIR"
echo "CLI directory: $CLI_DIR"

# Desmontar módulo si ya existe
if lsmod | grep -q nxp_simtemp; then
    echo "=== Desmontando módulo previo ==="
    sudo rmmod nxp_simtemp || true
fi

# Compilar módulo kernel
echo "=== Compilando módulo kernel ==="
make -C /lib/modules/$(uname -r)/build M="$KERNEL_DIR" modules

# Insertar módulo
echo "=== Insertando módulo ==="
sudo insmod "$KERNEL_DIR/nxp_simtemp.ko"

# Verificar que el módulo esté cargado
echo "=== Verificando módulo cargado ==="
lsmod | grep nxp_simtemp || true
dmesg | tail -n 10

# Compilar CLI
echo "=== Compilando CLI ==="
mkdir -p "$CLI_DIR/build"
g++ -I"$BASE_DIR/kernel" "$CLI_DIR/main.cpp" -o "$CLI_DIR/build/main"

# Ejecutar demo
echo "=== Ejecutando demo ==="
"$CLI_DIR/build/main" 

echo "=== Ejecutando demo --test==="
"$CLI_DIR/build/main" --test

# Desmontar módulo al final
echo "=== Desmontando módulo ==="
sudo rmmod nxp_simtemp || true

