#!/bin/bash

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
cd $INCLUDEOS_SRC/build/test

./unittests
