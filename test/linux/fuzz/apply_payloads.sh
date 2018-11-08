#!/bin/bash
set -e
export CC=clang-7.0
export CXX=clang++-7.0
#RUN=OFF $INCLUDEOS_PREFIX/bin/lxp-run
pushd build
cmake .. -DSANITIZE=ON -DLIBFUZZER=OFF -DPAYLOAD_MODE=ON
make -j4
popd
BINARY=build/"`cat build/binary.txt`"
$BINARY
