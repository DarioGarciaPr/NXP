#!/bin/bash
set -e   # si algo falla, se detiene

echo "================================="
echo " Compilando proyecto simtemp"
echo "================================="

# Ruta base del proyecto (directorio donde está este script)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# -------------------------------
# 1. Compilar el módulo del kernel
# -------------------------------
echo "[1/2] Compilando módulo kernel..."
cd "$PROJECT_ROOT/kernel"
make clean || true
make
echo "Kernel module compilado"

# -------------------------------
# 2. Compilar la app de usuario (CLI)
# -------------------------------
echo "[2/2] Compilando aplicación CLI..."
cd "$PROJECT_ROOT/user/cli"
make clean || true
make
echo "CLI compilado"

echo "================================="
echo " Build completado!"
echo "================================="

