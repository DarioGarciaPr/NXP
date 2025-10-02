#!/bin/bash
# reload.sh - recarga el módulo nxp_simtemp y muestra logs

MODULE_NAME="nxp_simtemp"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
KERNEL_DIR="$SCRIPT_DIR/../kernel"
MODULE_PATH="$KERNEL_DIR/${MODULE_NAME}.ko"
KDIR="/lib/modules/$(uname -r)/build"

echo "============================="
echo "Recargando módulo $MODULE_NAME"
echo "============================="

# 1. Cerrar cualquier reader bloqueado
echo "Cerrando procesos que usan /dev/$MODULE_NAME..."
PIDS=$(lsof /dev/$MODULE_NAME | awk 'NR>1 {print $2}')
if [ -n "$PIDS" ]; then
    echo "Procesos encontrados: $PIDS"
    echo "$PIDS" | xargs sudo kill -9
else
    echo "No hay procesos usando /dev/$MODULE_NAME"
fi

# 2. Quitar módulo antiguo (si existe)
if lsmod | grep -q "^$MODULE_NAME"; then
    echo "Quitando módulo antiguo..."
    sudo rmmod $MODULE_NAME || echo "No se pudo quitar módulo, verificar bloqueos"
fi

# 3. Compilar módulo
echo "Compilando módulo..."
make -C $KDIR M=$KERNEL_DIR modules || { echo "Error de compilación"; exit 1; }


# 4. Insertar módulo
echo "Insertando módulo..."
sudo insmod $MODULE_PATH || { echo "No se pudo insertar módulo"; exit 1; }

# 5. Mostrar últimos logs del módulo
echo "Últimos logs de dmesg:"
sudo dmesg | tail -n 20

echo "============================="
echo "Módulo recargado correctamente"
echo "============================="
