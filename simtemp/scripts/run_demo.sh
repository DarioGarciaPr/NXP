#!/bin/bash
set -e

ROOT_DIR=$(dirname "$0")/..
KERNEL_DIR="$ROOT_DIR/kernel"
CLI_DIR="$ROOT_DIR/user/cli"
DEV_NAME="simtemp"

echo "=== Preparando demo nxp_simtemp ==="

# Si el módulo ya está cargado, elimínalo
if lsmod | grep -q "^nxp_simtemp"; then
    echo "El módulo ya está cargado. Eliminando..."
    sudo rmmod nxp_simtemp
fi

# Insertar módulo
echo "Insertando módulo..."
sudo insmod "$KERNEL_DIR/nxp_simtemp.ko"

# Crear nodo de dispositivo si no existe
if [ ! -e "/dev/$DEV_NAME" ]; then
    echo "Nodo /dev/$DEV_NAME no encontrado. Creando..."
    MAJOR=$(grep "$DEV_NAME" /proc/misc | awk '{print $1}')
    if [ -z "$MAJOR" ]; then
        echo "Error: no se encontró major en /proc/misc"
        exit 1
    fi
    sudo mknod "/dev/$DEV_NAME" c "$MAJOR" 0
    sudo chmod 666 "/dev/$DEV_NAME"
fi
echo "/dev device detectado en /dev/$DEV_NAME"

# Verificar sysfs
if [ -d "/sys/class/misc/$DEV_NAME" ]; then
    echo "Atributos sysfs detectados en /sys/class/misc/$DEV_NAME"
else
    echo "Advertencia: Directorio sysfs /sys/class/misc/$DEV_NAME/ no encontrado."
fi

# Compilar CLI
echo "Compilando CLI..."
if [ ! -d "$CLI_DIR" ]; then
    echo "Error: no existe $CLI_DIR"
    exit 1
fi
cd "$CLI_DIR"
g++ -o main main.cpp
echo "CLI compilado."

# Ejecutar CLI en modo test/demo
echo "=== Ejecutando CLI (Test Mode) ==="
sudo ./main

# Limpiar
echo "Eliminando módulo..."
sudo rmmod nxp_simtemp
echo "=== Demo completado ==="

