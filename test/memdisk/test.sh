#!/bin/bash
source ../test_base

make SERVICE=Test DISK=sector0.disk FILES=nosector.cpp
start Test.img "Memdisk: Empty disk test"
make SERVICE=Test FILES=nosector.cpp clean

make SERVICE=Test DISK=sector2.disk FILES=twosector.cpp
start Test.img "Memdisk: Multiple sectors test"
make SERVICE=Test FILES=twosector.cpp clean

### BIG DISK TEST ###
rm -f big.disk
fallocate -l 131072000 big.disk # 256000 sectors
mkfs.fat big.disk
mkdir tmpdisk
sudo mount my.disk tmpdisk/
sudo cp banana.txt tmpdisk/
sync # Mui Importante

make SERVICE=Test DISK=big.disk FILES=bigdisk.cpp
export MEM="-m 256" 
start Test.img "Memdisk: Big disk test"
make SERVICE=Test FILES=bigdisk.cpp clean

sudo umount tmpdisk/
rm -f big.disk memdisk.o
