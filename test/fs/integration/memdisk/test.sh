#!/bin/bash
source ../test_base

make SERVICE=Test DISK=sector0.disk FILES=nosector.cpp
start Test.img "Memdisk: Empty disk test"
make SERVICE=Test FILES=nosector.cpp clean

make SERVICE=Test DISK=sector2.disk FILES=twosector.cpp
start Test.img "Memdisk: Multiple sectors test"
make SERVICE=Test FILES=twosector.cpp clean

# Create big disk
./bigdisk.sh
make SERVICE=Test DISK=big.disk FILES=bigdisk.cpp
export MEM="-m 256"
start Test.img "Memdisk: Big disk test"
make SERVICE=Test FILES=bigdisk.cpp clean

rm -f big.disk memdisk.o
