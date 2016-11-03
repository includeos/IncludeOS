#!/bin/bash

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}

cd $INCLUDEOS_SRC/test
make CXX=g++-5
./test.lest
