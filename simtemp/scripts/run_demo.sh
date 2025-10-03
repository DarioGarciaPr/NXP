#!/bin/bash
set -e

echo "Demo de simtemp"
echo "================================="

# 1/4 Recargando módulo
echo "[1/4] Recargando módulo..."
if lsmod | grep -q nxp_simtemp; then
    sudo rmmod nxp_simtemp
fi

make -C /lib/modules/$(uname -r)/build M=$(pwd)/kernel modules
sudo insmod kernel/nxp_simtemp.ko

# 2/4 Creando device node (si es necesario)
echo "[2/4] Creando device node..."
if [ ! -e /dev/nxp_simtemp ]; then
    sudo mknod /dev/nxp_simtemp c $(grep nxp_simtemp /proc/devices | awk '{print $1}') 0
fi

# 3/4 Compilando CLI
echo "[3/4] Compilando CLI..."
g++ -o ..user/cli/main ..user/cli/main.cpp

# 4/4 Ejecutando demo
echo "[4/4] Ejecutando demo..."
sudo ./user/cli/main

echo "Demo finalizada."

