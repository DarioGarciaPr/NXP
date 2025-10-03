#!/bin/bash
set -e

echo "[*] Inserting module..."
sudo insmod ../kernel/nxp_simtemp.ko || true

echo "[*] Running CLI in test mode..."
../user/cli/simtemp_cli --test

ret=$?

echo "[*] Removing module..."
sudo rmmod nxp_simtemp || true

exit $ret

