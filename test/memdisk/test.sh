#!/bin/bash
make DISK=sector0.disk FILES=nosector.cpp
./run.sh MemdiskTest.img

make DISK=sector2.disk FILES=twosector.cpp
./run.sh MemdiskTest.img
