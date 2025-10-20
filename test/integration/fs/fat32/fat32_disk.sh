#!/bin/bash
set -e #stop on first error
### FAT32 TEST DISK ###

DISK=my.disk
MOUNTDIR=tmpdisk

# If no arg supplied, setup disk
if [ $# -eq 0 ]
then

  # Remove disk if exists
  sudo rm -f $DISK
  # Preallocate space to a 4GB file
  sudo truncate -s 4G $DISK
  # Create FAT32 filesystem on "my.disk"
  sudo mkfs.fat $DISK

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
  rm -rf $MOUNTDIR

# If "clean" is supplied, clean up
elif [ $1 = "clean" ]
then
  echo "> Cleaning up FAT32 TEST DISK: $DISK"
  sudo rm -f $DISK
fi
