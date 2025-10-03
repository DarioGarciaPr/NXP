#!/bin/bash
set -e

MODULE_NAME=nxp_simtemp
KBUILD_DIR=/lib/modules/$(uname -r)/build
MODULE_DIR=$(pwd)

echo "=== Limpiando compilaciones previas ==="
make -C "$KBUILD_DIR" M="$MODULE_DIR" clean
rm -f ${MODULE_NAME}.ko ${MODULE_NAME}.mod.* *.o

echo "=== Compilando módulo ==="
make -C "$KBUILD_DIR" M="$MODULE_DIR" modules

echo "=== Verificando vermagic ==="
modinfo ${MODULE_NAME}.ko | grep vermagic

echo "=== Desmontando módulo previo (si existe) ==="
sudo rmmod ${MODULE_NAME} 2>/dev/null || true

echo "=== Cargando módulo ==="
sudo insmod ${MODULE_NAME}.ko

echo "=== Verificando que el módulo esté cargado ==="
lsmod | grep ${MODULE_NAME} || echo "Módulo no cargado"

echo "=== Últimos mensajes del kernel ==="
dmesg | tail -n 20

echo "=== Listo ==="

