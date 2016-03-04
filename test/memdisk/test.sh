#!/bin/bash
source ../test_base

make DISK=sector0.disk FILES=nosector.cpp > output.txt
start "Memdisk: Empty disk test"
make DISK=sector2.disk FILES=twosector.cpp > output.txt
start "Memdisk: Multiple sectors test"
