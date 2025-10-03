#!/bin/bash
# reload.sh - reload nxp_simtemp kernel module for testing

MODULE_NAME="nxp_simtemp"
KO_FILE="kernel/nxp_simtemp.ko"
DEV_FILE="/dev/simtemp"

echo "=== Reloading $MODULE_NAME module ==="

# Remove module if loaded
if lsmod | grep -q "^$MODULE_NAME"; then
    echo "Removing existing module..."
    sudo rmmod $MODULE_NAME
    sleep 1
fi

# Insert module
echo "Inserting module..."
sudo insmod $KO_FILE

# Check if module loaded
if lsmod | grep -q "^$MODULE_NAME"; then
    echo "$MODULE_NAME loaded successfully."
else
    echo "Error: $MODULE_NAME did not load."
    dmesg | tail -20
    exit 1
fi

# Verify /dev/simtemp
if [ -e $DEV_FILE ]; then
    echo "$DEV_FILE exists."
else
    echo "Warning: $DEV_FILE does not exist."
fi

# Show sysfs attributes
SYSFS_DIR="/sys/class/misc/simtemp"
if [ -d "$SYSFS_DIR" ]; then
    echo "Sysfs attributes:"
    ls -l $SYSFS_DIR
else
    echo "Warning: Sysfs directory $SYSFS_DIR not found."
fi

echo "=== Module reload complete ==="

