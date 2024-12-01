#!/bin/bash

IMAGE_NAME="storage_vgc.img"
IMAGE_SIZE="100M"
MOUNT_POINT="./mount"
LOOP_DEVICE="/dev/loop0"
DEVICE_LINK="./device-file"
BIN_DIR="./bin"

# Check for root privileges
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root. Use sudo."
   exit 1
fi

if [ -f "$IMAGE_NAME" ]; then
    echo "Existing image found. Overwriting..."
    rm -f "$IMAGE_NAME"
fi


dd if=/dev/zero of="$IMAGE_NAME" bs=1M count=100 status=progress
mkfs.ext4 "$IMAGE_NAME" > /dev/null


if [ ! -d "$MOUNT_POINT" ]; then
    mkdir "$MOUNT_POINT"
fi


LOOP_DEVICE=$(losetup -f)
losetup "$LOOP_DEVICE" "$IMAGE_NAME"

mount "$LOOP_DEVICE" "$MOUNT_POINT"

if [ ! -d "$BIN_DIR" ]; then
    echo "bin directory does not exist. Check $BIN_DIR"
    umount "$MOUNT_POINT"
    losetup -d "$LOOP_DEVICE"
    exit 1
fi

cp -r "$BIN_DIR" "$MOUNT_POINT"
umount "$MOUNT_POINT"
losetup -d "$LOOP_DEVICE"

ln -sf "$LOOP_DEVICE" "$DEVICE_LINK"

echo "Disk image $IMAGE_NAME created, formatted, and populated with bin directory successfully."
