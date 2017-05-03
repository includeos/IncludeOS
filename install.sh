#! /bin/bash

[ -z ${INCLUDEOS_PREFIX} ] && export INCLUDEOS_PREFIX="/usr/local";

# Install deps
pushd deps
mkdir -p tmp

INCLUDES=$INCLUDEOS_PREFIX/includeos/include/
LIBS=$INCLUDEOS_PREFIX/includeos/lib/

# LiveUpdate
tar -xf liveupdate.tar -C ./tmp
echo -e ">>> Installing liveupdate.hpp in $INCLUDES"
cp ./tmp/liveupdate.hpp $INCLUDES
echo -e ">>> Installing libliveupdate.a in $LIBS"
cp ./tmp/libliveupdate.a $LIBS

rm -rf tmp
popd

# cmake install
mkdir -p build
pushd build
cmake .. -DCMAKE_INSTALL_PREFIX=$INCLUDEOS_PREFIX/includeos
make install
popd