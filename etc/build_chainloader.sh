#!/bin/bash

set -e

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
INCLUDEOS_PREFIX=${INCLUDEOS_PREFIX-/usr/local}
num_jobs=${num_jobs-"-j 4"}

CHAINLOAD_LOC=$INCLUDEOS_SRC/src/chainload

export PLATFORM=x86_nano
export ARCH=i686

echo -e "\n\n>>> Building chainloader deps, $ARCH / $PLATFORM"

if [ "Darwin" = "$SYSTEM" ]; then
	if ! ./etc/install_dependencies_macos.sh; then
		printf "%s\n" ">>> Sorry <<<"\
					 "Could not install dependencies"
	fi
fi

$INCLUDEOS_SRC/etc/install_from_bundle.sh

echo -e "\n\n>>> Building chainloader"

mkdir -p $CHAINLOAD_LOC/build
pushd $CHAINLOAD_LOC/build

cmake .. -DCMAKE_INSTALL_PREFIX=$INCLUDEOS_PREFIX
make $num_jobs install

popd
