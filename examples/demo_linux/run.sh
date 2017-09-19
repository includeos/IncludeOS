#!/bin/bash
mkdir -p build
pushd build
cmake ..
make -j4
popd
sudo mknod /dev/net/tap c 10 200
sudo ./build/demo_example
