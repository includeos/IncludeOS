#!/bin/bash
set -e
mkdir -p build
pushd build
cmake ..
make -j4
popd
set +e
sudo mknod /dev/net/tap c 10 200
sudo ./build/tcp_perf
