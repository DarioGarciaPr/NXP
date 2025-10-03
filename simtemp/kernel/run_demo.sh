#!/bin/bash
set -e

MODULE_NAME=nxp_simtemp
KBUILD_DIR=/lib/modules/$(uname -r)/build
MODULE_DIR=$(pwd)
DEVICE_NODE=/dev/$MODULE_NAME
CLI_EXE=../user/cli/main  # Ajusta si tu CLI está en otra ruta

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

echo "=== Creando device node si no existe ==="
if [ ! -e "$DEVICE_NODE" ]; then
    MAJOR=$(grep $MODULE_NAME /proc/devices | awk '{print $1}')
    if [ -z "$MAJOR" ]; then
        echo "No se encontró major number, usando misc_register en el driver"
    else
        sudo mknod "$DEVICE_NODE" c $MAJOR 0
        sudo chmod 666 "$DEVICE_NODE"
    fi
fi

echo "=== Verificando que el módulo esté cargado ==="
lsmod | grep ${MODULE_NAME} || echo "Módulo no cargado"

echo "=== Últimos mensajes del kernel ==="
dmesg | tail -n 20

echo "=== Ejecutando demo CLI ==="
if [ -x "$CLI_EXE" ]; then
    "$CLI_EXE"
else
    echo "No se encontró CLI en $CLI_EXE. Compílalo primero."
fi

echo "=== Demo completada ==="

