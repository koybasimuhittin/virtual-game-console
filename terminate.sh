#!/bin/bash

IMAGE_NAME="storage_vgc.img"
MOUNT_POINT="./mount"
DEVICE_LINK="./device-file"

# Check for root privileges
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root. Use sudo."
    exit 1
fi

LOOP_DEVICE=$(losetup -j "$IMAGE_NAME" | cut -d':' -f1)

if [ -z "$LOOP_DEVICE" ]; then
    echo "Loop device cannot be found for '$IMAGE_NAME'."
    exit 1
fi
sudo umount "$MOUNT_POINT"
sudo losetup -d "$LOOP_DEVICE"
rm -f "$DEVICE_LINK"
exit 0

echo "Virtual disk has been unmounted, detached, and cleaned up successfully."
