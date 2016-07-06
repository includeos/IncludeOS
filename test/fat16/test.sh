#!/bin/bash

### FAT16 TEST ###

DISK=my.disk
MOUNTDIR=tmpdisk

# If no args supplied, create a fat disk and fill with data
if [ $# -eq 0 ]
then

  # Remove disk if exists
  rm -f $DISK
  # Create "my.disk" with 16500 blocks (8 MB)
  dd if=/dev/zero of=$DISK count=16500
  # Create FAT filesystem on "my.disk"
  mkfs.fat $DISK

  # Create mount dir
  mkdir -p $MOUNTDIR
  sudo mount $DISK $MOUNTDIR/
  sudo cp banana.txt $MOUNTDIR/
  sync # Mui Importante
  sudo umount $MOUNTDIR/

# If "clean" is supplied, clean up
elif [ $1 = "clean" ]
then
  echo "> Cleaning up after FAT16 test"
  make clean
  rm -rf memdisk.o $MOUNTDIR/ $DISK
fi
