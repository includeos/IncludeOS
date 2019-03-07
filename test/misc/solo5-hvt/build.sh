#!/bin/bash
set -e

pushd $INCLUDEOS_SRC/examples/demo_service/build_solo5-hvt
make
popd
