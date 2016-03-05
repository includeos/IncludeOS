#!/bin/bash
source ../test_base

dd if=/dev/zero of=my.disk count=16500
mkfs.fat my.disk

make SERVICE=Test DISK=my.disk FILES=test.cpp
start Test.img "Memdisk: Empty disk test"
make SERVICE=Test FILES=test.cpp clean
rm memdisk.o
