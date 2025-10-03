#!/bin/bash
set -e

echo "=============================="
echo "Reconstruyendo nxp_simtemp..."
echo "Kernel actual: $(uname -r)"
echo "=============================="

# 1. Limpiar compilaciones previas
echo "[1/5] Limpiando compilaciones previas..."
make -C /lib/modules/$(uname -r)/build M=$(pwd) clean
rm -f nxp_simtemp.ko nxp_simtemp.mod.* *.o

# 2. Compilar módulo
echo "[2/5] Compilando módulo..."
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# 3. Verificar vermagic
echo "[3/5] Verificando vermagic..."
V=$(modinfo nxp_simtemp.ko | grep vermagic | awk '{print $2}')
K=$(uname -r)
echo "vermagic del módulo: $V"
echo "kernel en ejecución: $K"

if [[ "$V" != "$K" ]]; then
    echo "ERROR: vermagic no coincide con kernel. No se puede cargar el módulo."
    exit 1
fi

# 4. Eliminar módulo si ya estaba cargado
echo "[4/5] Eliminando módulo previo (si existe)..."
sudo rmmod nxp_simtemp 2>/dev/null || true

# 5. Cargar módulo
echo "[5/5] Cargando módulo..."
sudo insmod nxp_simtemp.ko

echo "Módulo cargado correctamente."
lsmod | grep nxp_simtemp
echo "Últimos mensajes del kernel:"
dmesg | tail -n 20

