#!/bin/bash

### BIG DISK CREATION ###
DISK=big.disk
MOUNTDIR=tmpdisk

rm -f $DISK
echo "> Creating big disk"
fallocate -l 131072000 $DISK # 256000 sectors
mkfs.fat $DISK
# mkdir -p $MOUNTDIR
# sudo mount $DISK $MOUNTDIR
# sudo cp banana.txt $MOUNTDIR
# sync # Mui Importante
# sudo umount $MOUNTDIR
# rmdir $MOUNTDIR
