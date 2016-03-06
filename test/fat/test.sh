#!/bin/bash
source ../test_base

dd if=/dev/zero of=my.disk count=16500
mkfs.fat my.disk
mkdir tmpdisk
sudo mount my.disk tmpdisk/
sudo cp Makefile tmpdisk/
sync # Mui Importante

make SERVICE=Test DISK=my.disk FILES=fat16.cpp
start Test.img "FAT: FAT16 test"
make SERVICE=Test FILES=fat16.cpp clean
rm memdisk.o

sudo umount tmpdisk/
rmdir tmpdisk/
rm my.disk
