#!/bin/bash
set -e
export CC=clang-7.0
export CXX=clang++-7.0
RUN=OFF $INCLUDEOS_PREFIX/bin/lxp-run
BINARY=build/"`cat build/binary.txt`"
# use -help=1 to see parameters to libfuzzer
$BINARY -max_len=120
