#!/bin/bash
set -e #stop on first error
### DISK CREATION ###
ROOTDIR=tmpdisk

# Set local directory to first param if provided, default to memdisk
CONTENT=$1
LOCALDIR=$2
LOCALDIR=${LOCALDIR:-memdisk}

DISK=$LOCALDIR.disk

echo ">> Creating disk image from $LOCALDIR to $LOCALDIR.disk"
mkdir -p $ROOTDIR
cp -r $CONTENT/* $ROOTDIR
sync
diskbuilder -o $DISK $ROOTDIR
echo "DONE creating"