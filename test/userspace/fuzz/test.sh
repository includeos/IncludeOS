#!/bin/bash
set -e
export CC=gcc-7
export CXX=g++-7
RUN=OFF $INCLUDEOS_PREFIX/bin/lxp-run

if [ $? == 0 ]; then
  echo ">>> Building fuzzer success!"
else
  exit 1
fi
