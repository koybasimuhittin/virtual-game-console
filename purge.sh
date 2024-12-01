#!/bin/bash

IMAGE_NAME="storage_vgc.img"
MOUNT_POINT="./mount"
DEVICE_LINK="./device-file"

./terminate.sh
rm -rf "$IMAGE_NAME"

echo "Purge completed successfully."
exit 0
