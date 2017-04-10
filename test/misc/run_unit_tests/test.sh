#!/bin/bash

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
pushd $INCLUDEOS_SRC/build/unittests
./unittests --pass
popd
