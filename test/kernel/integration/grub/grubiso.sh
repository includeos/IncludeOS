#!/bin/bash
set -e

LOCAL_DISK="$(mktemp -d)"
SERVICE=$1
GRUBIMG=grub.iso

echo "Building $GRUBIMG..."
# create grub.iso
mkdir -p "$LOCAL_DISK"/boot/grub
cp "$SERVICE" "$LOCAL_DISK"/boot/service
cp grub.cfg "$LOCAL_DISK"/boot/grub
grub-mkrescue -o "$GRUBIMG" "$LOCAL_DISK"
rm -vrf "$LOCAL_DISK"
