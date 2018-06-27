#!/bin/bash

set -e

export GPROF="OFF"
export DBG="OFF"

export num_jobs=${num_jobs:--j32}

function make_linux(){
  pushd $INCLUDEOS_SRC/linux/build
  cmake -DGPROF=$GPROF -DDEBUGGING=$DBG ..
  make $num_jobs install
  popd
}

function make_service(){
  mkdir -p build
  pushd build
  cmake -DGPROF=$GPROF -DDEBUGGING=$DBG ..
  make $num_jobs
  popd
}

if [ -z "$1" ]
then
  GPROF="OFF"
elif [ "$1" == "gprof" ]
then
  GPROF="ON"
fi


make_linux
make_service

#sudo mknod /dev/net/tap c 10 200
BINARY=build/"`cat build/binary.txt`"
sudo $BINARY
