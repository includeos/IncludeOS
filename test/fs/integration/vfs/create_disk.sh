#!/bin/bash

### DISK CREATION ###
MOUNTDIR=tmpdisk

# Set local directory to first param if provided, default to memdisk
LOCALDIR=$1
LOCALDIR=${LOCALDIR:-memdisk}

DISK=$LOCALDIR.disk

rm -f $DISK
echo "> Creating disk image from $LOCALDIR to $LOCALDIR.disk"
fallocate -l 1048576 $DISK # 256000 sectors
mkfs.fat $DISK
mkdir -p $MOUNTDIR
sudo mount $DISK $MOUNTDIR
sudo cp -r memdisk/* $MOUNTDIR
sync # Mui Importante
sudo umount $MOUNTDIR
rmdir $MOUNTDIR
