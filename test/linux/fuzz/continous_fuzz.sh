#!/bin/bash
set -e
export CC=clang-9
export CXX=clang++-9
RUN=OFF $INCLUDEOS_PREFIX/bin/lxp-run
BINARY=build/"`cat build/binary.txt`"
# use -help=1 to see parameters to libfuzzer
LD_LIBRARY_PATH=$HOME/llvm/install/lib $BINARY -max_len=120 -rss_limit_mb=512
