#!/bin/bash
set -e
#export INCLUDEOS_PREFIX=$HOME/includeos

# set default compiler if not set
CC=${CC:-clang-5.0}
CXX=${CXX:-clang++-5.0}

read -rsp $'Press enter to continue...\n'

# install chainloader
mkdir -p $INCLUDEOS_PREFIX/includeos
curl -L -o $INCLUDEOS_PREFIX/includeos/chainloader https://github.com/fwsGonzo/barebones/releases/download/v0.9-cl/chainloader

# cleanup old build
rm -rf build_x86_64

mkdir -p build_x86_64
pushd build_x86_64
cmake .. -DCMAKE_INSTALL_PREFIX=$INCLUDEOS_PREFIX
make -j32
make install
popd
