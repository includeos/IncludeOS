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

[ ! -v INCLUDEOS_SRC ] && export INCLUDEOS_SRC=`pwd`
[ ! -v INCLUDEOS_INSTALL_LOC ] && export INCLUDEOS_INSTALL_LOC=$HOME
export INCLUDEOS_HOME=$INCLUDEOS_INSTALL_LOC/IncludeOS_install

# Get the latest tag from IncludeOS repo
pushd $INCLUDEOS_SRC
git pull --tags
tag=`git describe --abbrev=0`
popd 

filename_tag=`echo $tag | tr . -`
filename="IncludeOS_install_"$filename_tag".tar.gz"

# If the tarball exists, use that 
if [ -e $filename ] 
then
    echo ">>> IncludeOS tarball exists - extracting..."
    tar -C $INCLUDEOS_INSTALL_LOC -xzf $filename
else    
    # Download from GitHub API    
    # IF PRIVATE:
    echo -n "Enter github username: "
    export git_user="git"
    read git_user
    
    
    JSON=`curl -u $git_user https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`
    ASSET=`echo $JSON | $INCLUDEOS_SRC/etc/get_latest_binary_bundle_asset.py`
    ASSET_URL=https://api.github.com/repos/hioa-cs/IncludeOS/releases/assets/$ASSET

    curl -H "Accept: application/octet-stream" -L -o $filename -u $git_user $ASSET_URL
    
    echo ">>> Fetched IncludeOS tarball from GitHub - extracting..."
    tar -C $INCLUDEOS_INSTALL_LOC -xzf $filename    
fi

# Install dependencies
DEPENDENCIES="make clang-3.6 nasm bridge-utils qemu"
echo ">>> Installing dependencies (requires sudo):"
echo "    Packages: $DEPENDENCIES"
sudo apt-get install $DEPENDENCIES

echo ">>> Compiling the vmbuilder"
pushd $INCLUDEOS_SRC/vmbuild
make
cp vmbuild $INCLUDEOS_HOME/
popd

echo ">>> Creating a virtual network, i.e. a bridge. (Requires sudo)"
sudo $INCLUDEOS_SRC/etc/create_bridge.sh

mkdir -p $INCLUDEOS_HOME/etc
cp $INCLUDEOS_SRC/etc/qemu-ifup $INCLUDEOS_HOME/etc/




