#!/bin/bash
set -e #stop on first error
### FAT32 TEST DISK ###

DISK=my.disk
ROOTDIR=tmpdisk

# Create ROOTDIR and mount
mkdir -p $ROOTDIR
# Copy banana.txt onto disk
cp banana.txt $ROOTDIR/
# Create deep nested directory and copy banana.txt into dir
mkdir -p $ROOTDIR/dir1/dir2/dir3/dir4/dir5/dir6
cp banana.txt $ROOTDIR/dir1/dir2/dir3/dir4/dir5/dir6/
sync # Mui Importante

diskbuilder -o $DISK $ROOTDIR