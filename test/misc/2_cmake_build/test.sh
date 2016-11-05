#!/bin/bash

# Build using cmake

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}

cd $INCLUDEOS_SRC

# Remove the build dir if it exists
rm -rf build

mkdir build
cd build/

CC='clang-3.8' CXX='clang++-3.8' cmake ..
make
