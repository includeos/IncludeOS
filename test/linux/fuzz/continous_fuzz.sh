#!/bin/bash
set -e
export CC=clang-7
export CXX=clang++-7
RUN=OFF $INCLUDEOS_PREFIX/bin/lxp-run
BINARY=build/"`cat build/binary.txt`"
# maybe run with LLVM symbolizer
$BINARY
