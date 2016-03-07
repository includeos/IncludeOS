#!/bin/bash
source ../test_base
rm -f my.disk

### FAT16 TEST ###
dd if=/dev/zero of=my.disk count=16500
mkfs.fat my.disk
mkdir tmpdisk
sudo mount my.disk tmpdisk/
sudo cp banana.txt tmpdisk/
sync # Mui Importante

make SERVICE=Test DISK=my.disk FILES=fat16.cpp
start Test.img "FAT: FAT16 test"
make SERVICE=Test FILES=fat16.cpp clean
rm memdisk.o

sudo umount tmpdisk/
rmdir tmpdisk/
rm my.disk
