#!/bin/bash
# reload.sh - Recarga el módulo nxp_simtemp

set -e

MODULE_NAME=nxp_simtemp
MODULE_PATH=../kernel

echo "Recargando módulo $MODULE_NAME..."

# Elimina el módulo si ya está cargado
if lsmod | grep -q "^$MODULE_NAME"; then
    echo "  -> Módulo ya cargado, eliminando..."
    sudo rmmod $MODULE_NAME || true
fi

# Compila el módulo
echo "  -> Compilando módulo..."
make -C /lib/modules/$(uname -r)/build M=$(realpath $MODULE_PATH) modules

# Inserta el módulo
echo "  -> Insertando módulo..."
sudo insmod $MODULE_PATH/$MODULE_NAME.ko

# Confirma que el device node fue creado automáticamente
DEV_NODE="/dev/$MODULE_NAME"
if [ -e "$DEV_NODE" ]; then
    echo "  -> Device node $DEV_NODE creado automáticamente"
else
    echo "  -> ATENCIÓN: Device node no encontrado. Verifica misc_register en el driver"
fi

echo "Módulo $MODULE_NAME recargado correctamente."

