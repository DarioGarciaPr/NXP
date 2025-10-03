#!/bin/bash
# run_demo.sh
# Script para cargar módulo y ejecutar demo CLI

set -e  # salir si hay algún error

# Obtener directorio del script
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Paths
KERNEL_MODULE="$SCRIPT_DIR/../kernel/nxp_simtemp.ko"
CLI_BIN="$SCRIPT_DIR/../user/cli/main"

echo "=== Desmontando módulo previo (si existe) ==="
sudo rmmod nxp_simtemp 2>/dev/null || true

echo "=== Insertando módulo ==="
sudo insmod "$KERNEL_MODULE"

echo "=== Verificando que el módulo esté cargado ==="
lsmod | grep nxp_simtemp || echo "Módulo no cargado"

echo "=== Ejecutando demo CLI (C++) ==="
if [ ! -f "$CLI_BIN" ]; then
    echo "El binario CLI no existe. Compilando main.cpp..."
    g++ -o "$CLI_BIN" "$(dirname "$CLI_BIN")/main.cpp"
fi

"$CLI_BIN"

echo "=== Demo completada ==="

