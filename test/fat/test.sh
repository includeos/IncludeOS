#!/bin/bash
source ../test_base
mkdir tmpdisk

### FAT16 TEST ###
rm -f my.disk
dd if=/dev/zero of=my.disk count=16500
mkfs.fat my.disk
sudo mount my.disk tmpdisk/
sudo cp banana.txt tmpdisk/
sync # Mui Importante
sudo umount tmpdisk/

make SERVICE=Test DISK=my.disk FILES=fat16.cpp
start Test.img "FAT: FAT16 test"
make SERVICE=Test FILES=fat16.cpp clean
rm -f memdisk.o my.disk

### FAT16 TEST ###
fallocate -l 2147483648 my.disk
mkfs.fat my.disk
mkdir tmpdisk
sudo mount my.disk tmpdisk/
sudo cp banana.txt tmpdisk/
sudo mkdir -p tmpdisk/dir1/dir2/dir3/dir4/dir5/dir6
sudo cp banana.txt tmpdisk/dir1/dir2/dir3/dir4/dir5/dir6/
sync # Mui Importante
sudo umount tmpdisk/

export QEMU_EXTRA=" -drive file=my.disk,if=ide,media=disk"
make SERVICE=Test FILES=fat32.cpp
start Test.img "FAT: FAT32 test"
make SERVICE=Test FILES=fat32.cpp clean
rm -f memdisk.o my.disk

rmdir tmpdisk/
