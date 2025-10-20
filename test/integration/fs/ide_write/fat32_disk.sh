#!/bin/bash

### FAT32 TEST DISK ###

DISK=my.disk
MOUNTDIR=tmpdisk

# If no arg supplied, setup disk
if [ $# -eq 0 ]
then

  # Remove disk if exists
  rm -f $DISK
  # Preallocate space to file
  truncate -s 1M $DISK
  # Create FAT32 filesystem on "my.disk"
  mkfs.fat $DISK

# If "clean" is supplied, clean up
elif [ $1 = "clean" ]
then
  echo "> Cleaning up TEST DISK: $DISK"
  rm -f $DISK
fi
