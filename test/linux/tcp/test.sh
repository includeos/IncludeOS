#!/bin/bash
set -e
$INCLUDEOS_PREFIX/bin/lxp-run | grep 'Server received'

if [ $? == 0 ]; then
  echo ">>> Linux Userspace TCP test success!"
else
  exit 1
fi
