#!/bin/bash

### FAT32 TEST DISK ###

DISK=my.disk
MOUNTDIR=tmpdisk

# If no arg supplied, setup disk
if [ $# -eq 0 ]
then

  # Remove disk if exists
  rm -f $DISK
  # Preallocate space to a 2 GB file
  fallocate -l 4000000000 $DISK
  # Create FAT32 filesystem on "my.disk"
  mkfs.fat $DISK

  # Create mountdir and mount
  mkdir -p $MOUNTDIR
  sudo mount -o rw $DISK $MOUNTDIR/
  # Copy banana.txt onto disk
  sudo cp banana.txt $MOUNTDIR/
  # Create deep nested directory and copy banana.txt into dir
  sudo mkdir -p $MOUNTDIR/dir1/dir2/dir3/dir4/dir5/dir6
  sudo cp banana.txt $MOUNTDIR/dir1/dir2/dir3/dir4/dir5/dir6/
  sync # Mui Importante
  sudo umount $MOUNTDIR/
  rmdir $MOUNTDIR

# If "clean" is supplied, clean up
elif [ $1 = "clean" ]
then
  echo "> Cleaning up FAT32 TEST DISK: $DISK"
  rm -f $DISK
fi
