#!/bin/bash

# Build using cmake

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}

cd $INCLUDEOS_SRC

mkdir build
cd build/
CC='clang-3.8' CXX='clang++-3.8' cmake ..
make
