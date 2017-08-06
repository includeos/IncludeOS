#!/bin/bash
INSTALL=`pwd`
mkdir -p build
pushd build
cmake .. -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL
make install
popd
