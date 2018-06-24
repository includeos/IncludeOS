#!/bin/bash
pushd build
make -j8
popd
sudo gdb build/websockets
