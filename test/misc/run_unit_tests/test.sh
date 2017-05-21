#!/bin/bash

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
ARCH=${ARCH:-x86_64}

pushd $INCLUDEOS_SRC/build_$ARCH/unittests
./unittests --pass
popd
