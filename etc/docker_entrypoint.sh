#!/bin/bash

# entrypoint.sh exports the correct clang version

# fixuid gives the user in the container the same uid:gid as specified when running
# docker run with --user uid:gid. This is to prevent file permission errors
eval $( fixuid &> /dev/null )

pushd / > /dev/null
if version=$(grep -oP 'CLANG_VERSION_MIN_REQUIRED="\K[^"]+' use_clang_version.sh); then :
elif version=$(grep -oP 'CLANG_VERSION_MIN_REQUIRED="\K[^"]+' install_dependencies_linux.sh); then :
elif version=$(grep -oP 'CLANG_VERSION="\K[^"]+' install_dependencies_linux.sh); then :
else version=3.8
fi
export CC=clang-$version
export CXX=clang++-$version
popd > /dev/null

# Execute any command following entrypoint.sh
exec "$@"
