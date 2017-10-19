#!/bin/bash
set -e
git submodule update --init --recursive
mkdir -p build
pushd build
cmake ..
make -j4
popd
echo "Done! Type 'boot .' to start."
