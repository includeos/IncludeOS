#!/bin/bash

# Install the IncludeOS libraries (i.e. IncludeOS_home) from binary bundle
# ...as opposed to building them all from scratch, which takes a long time
#
#
# OPTIONS:
#
# Location of the IncludeOS repo:
# $ export INCLUDEOS_SRC=your/github/cloned/IncludeOS
#
# Parent directory of where you want the IncludeOS libraries (i.e. IncludeOS_home)
# $ export INCLUDEOS_INSTALL_LOC=parent/folder/for/IncludeOS/libraries i.e.

[ ! -v INCLUDEOS_SRC ] && export INCLUDEOS_SRC=$(readlink -f "$(dirname "$0")/")
[ ! -v INCLUDEOS_INSTALL_LOC ] && export INCLUDEOS_INSTALL_LOC=$HOME
export INCLUDEOS_HOME=$INCLUDEOS_INSTALL_LOC/IncludeOS_install

# Get the latest tag from IncludeOS repo
echo -e "\n\n>>> Updating git-tags "
git fetch --tags https://github.com/hioa-cs/IncludeOS.git master
tag=`git describe --abbrev=0`
echo "Latest tag found: $tag"

filename_tag=`echo $tag | tr . -`
filename="IncludeOS_install_"$filename_tag".tar.gz"
echo "Full filename: $filename"

# If the tarball exists, use that
if [ -e $filename ]
then
    echo -e "\n\n>>> IncludeOS tarball exists - extracting to $INCLUDEOS_INSTALL_LOC"
else
    # Download from GitHub API
    echo -e "\n\n>>> Getting the ID of the latest release from GitHub"
    JSON=`curl https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`
    ASSET=`echo $JSON | $INCLUDEOS_SRC/etc/get_latest_binary_bundle_asset.py`
    ASSET_URL=https://api.github.com/repos/hioa-cs/IncludeOS/releases/assets/$ASSET

    echo -e "\n\n>>> Downloading latest IncludeOS release tarball from GitHub"
    curl -H "Accept: application/octet-stream" -L -o $filename $ASSET_URL
fi

# Extracting the downloaded tarball
echo -e "\n\n>>> Fetched tarball - extracting to $INCLUDEOS_INSTALL_LOC/IncludeOS_install"
gunzip $filename -c | tar -C $INCLUDEOS_INSTALL_LOC -xf -   # Pipe gunzip to tar

# Install submodules
echo -e "\n\n>>> Installing submodules"
pushd $INCLUDEOS_SRC
git submodule init
git submodule update
popd

# Build IncludeOS
echo -e "\n\n>>> Building IncludeOS"
pushd $INCLUDEOS_SRC/src
make -j
make install
popd

# Compile vmbuilder
echo -e "\n\n>>> Compiling the vmbuilder, which makes a bootable vm out of your service"
pushd $INCLUDEOS_SRC/vmbuild
make
cp vmbuild $INCLUDEOS_HOME/
popd

# Copy scripts for running qemu, creating a memdisk
$INCLUDEOS_SRC/etc/copy_scripts.sh

echo -e "\n\n>>> Done! IncludeOS bundle downloaded and installed"