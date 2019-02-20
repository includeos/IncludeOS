#!/bin/bash
LOCAL_DISK=temp_disk
SERVICE=$1
GRUBIMG=grub.iso
set -e

echo "Building $GRUBIMG..."
# create grub.iso
mkdir -p $LOCAL_DISK/boot/grub
cp $SERVICE $LOCAL_DISK/boot/service
cp grub.cfg $LOCAL_DISK/boot/grub
grub-mkrescue -d /usr/lib/grub/i386-pc -o $GRUBIMG $LOCAL_DISK
rm -rf $LOCAL_DISK
