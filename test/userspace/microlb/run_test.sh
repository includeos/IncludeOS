#!/bin/bash
set -e
export CC=gcc-7
export CXX=g++-7
AS_ROOT=ON $INCLUDEOS_PREFIX/bin/lxp-run
