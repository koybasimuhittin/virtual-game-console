#!/bin/bash

IMAGE_NAME="storage_vgc.img"
MOUNT_POINT="./mount"
DEVICE_LINK="./device-file"

# Check for root privileges
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root. Use sudo."
   exit 1
fi

if [ ! -d "$MOUNT_POINT" ]; then
    echo "Creating mount directory at $MOUNT_POINT"
    mkdir -p "$MOUNT_POINT"
fi


LOOP_DEVICE=$(losetup -f)

if [ -z "$LOOP_DEVICE" ]; then
    echo "No available loop devices."
    exit 1
fi

losetup "$LOOP_DEVICE" "$IMAGE_NAME"
mount "$LOOP_DEVICE" "$MOUNT_POINT"

ln -sf "$LOOP_DEVICE" "$DEVICE_LINK"

chmod +x "$MOUNT_POINT/"*

exit 0
