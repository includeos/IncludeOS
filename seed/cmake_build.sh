#!/bin/bash
INSTALL=`pwd`
mkdir -p build
pushd build
cmake .. -DINCLUDEOS_INSTALL=$INCLUDEOS_INSTALL -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL
make install
popd
