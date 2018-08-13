#!/bin/bash
mkdir -p build
pushd build
cmake ..
make -j8
popd
dd if=build/liveupdate > /dev/tcp/10.0.0.42/666
