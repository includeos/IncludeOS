#!/bin/bash
set -e
export CC=gcc-7
export CXX=g++-7
RUN="OFF" $INCLUDEOS_PREFIX/bin/lxp-run
echo ">>> Linux S2N stream test built!"
