#!/bin/bash
set -e
export CC=gcc-7
export CXX=g++-7
$INCLUDEOS_PREFIX/bin/lxp-pgo | grep 'Server received'

if [ $? == 0 ]; then
  echo ">>> Linux Userspace TCP test success!"
else
  exit 1
fi
