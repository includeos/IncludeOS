#!/bin/bash
BFOLD=build
mkdir -p $BFOLD
pushd $BFOLD
cmake ..
make -j8
popd
dd if=$BFOLD/liveupdate > /dev/tcp/10.0.0.42/666
