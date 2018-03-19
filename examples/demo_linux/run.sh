#!/bin/bash
sudo mknod /dev/net/tap c 10 200
set -e
mkdir -p build
pushd build
cmake ..
make -j4
popd
sudo ./build/linux_demo
