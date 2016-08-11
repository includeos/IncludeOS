#!/bin/bash
source ../test_base
mkdir tmpdisk

### FAT16 TEST ###
rm -f my.disk
dd if=/dev/zero of=my.disk count=6500
mkfs.fat my.disk
sudo mount my.disk tmpdisk/
sudo cp banana.txt tmpdisk/
sudo mkdir -p tmpdisk/dir1/dir2/dir3
sync # Mui Importante
sudo umount tmpdisk/

make SERVICE=Test DISK=my.disk FILES=term.cpp
start Test.img "Term: Terminal test"
make SERVICE=Test FILES=term.cpp clean
rm -f memdisk.o my.disk
rmdir tmpdisk/
