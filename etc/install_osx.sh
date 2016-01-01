#! /bin/bash

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

[[ -z $INCLUDEOS_SRC ]] && export INCLUDEOS_SRC=`pwd`
[[ -z $INCLUDEOS_INSTALL_LOC ]] && export INCLUDEOS_INSTALL_LOC=$HOME
export INCLUDEOS_HOME=$INCLUDEOS_INSTALL_LOC/IncludeOS_install
mkdir -p $INCLUDEOS_HOME

# Install dependencies
#DEPENDENCIES="curl make clang-3.6 nasm bridge-utils qemu"
#echo ">>> Installing dependencies (requires sudo):"
#echo "    Packages: $DEPENDENCIES"
#sudo apt-get update
#sudo apt-get install -y $DEPENDENCIES


echo ">>> Updating git-tags "
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
    echo -e "\n\n>>> IncludeOS tarball exists - extracting to $INCLUDEOS_INSTALL_LOC"
    #gzip -c $filename | tar -C $INCLUDEOS_INSTALL_LOC xopf -
else    
    echo -e "\n\n>>> Downloading IncludeOS release tarball from GitHub"
    # Download from GitHub API    
    if [ "$1" = "-oauthToken" ]
    then
        oauthToken=$2
        echo -e "\n\n>>> Getting the ID of the latest release from GitHub"
        JSON=`curl -u $git_user:$oauthToken https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`
    else
        # IF PRIVATE:
        echo -n "Enter github username: "
        export git_user="git"
        read git_user

        echo -e "\n\n>>> Getting the ID of the latest release from GitHub"
        JSON=`curl -u $git_user https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`
    fi
    ASSET=`echo $JSON | $INCLUDEOS_SRC/etc/get_latest_binary_bundle_asset.py`
    ASSET_URL=https://api.github.com/repos/hioa-cs/IncludeOS/releases/assets/$ASSET

    echo -e "\n\n>>> Getting the latest release bundle from GitHub"
    if [ "$1" = "-oauthToken" ]
    then
        curl -H "Accept: application/octet-stream" -L -o $filename -u $git_user:$oauthToken $ASSET_URL
    else
        curl -H "Accept: application/octet-stream" -L -o $filename -u $git_user $ASSET_URL
    fi
    
    echo -e "\n\n>>> Fetched tarball - extracting to $INCLUDEOS_INSTALL_LOC"
    #gzip -c $filename | tar -C $INCLUDEOS_INSTALL_LOC xopf -     
fi

# Install
gzip -c $filename | tar xopf - -C $INCLUDEOS_INSTALL_LOC

### Install Binutils - needed for linking ###

echo -e "\n\n>>> Installing Binutils (archiver and linker)"
export INCLUDEOS_BUILD=$INCLUDEOS_INSTALL_LOC/IncludeOS_build
mkdir -p $INCLUDEOS_BUILD

## Download
BINUTILS_RELEASE=binutils-2.25
filename_binutils=$BINUTILS_RELEASE".tar.gz"
if [ -e $INCLUDEOS_BUILD/$filename_binutils ]
then 
    echo -e "\n>> $BINUTILS_RELEASE already downloaded."
else
    echo -e "\n>> Downloading $BINUTILS_RELEASE"
    curl https://ftp.gnu.org/gnu/binutils/$filename_binutils -o $INCLUDEOS_BUILD/$filename_binutils # IncludeOS_build/binutils-2.25.tar.gz
    ## Unzip
    gzip -c $INCLUDEOS_BUILD/$filename_binutils | tar xopf - -C $INCLUDEOS_BUILD
fi

export BINUTILS_DIR=$INCLUDEOS_BUILD/binutils
LINKER_PREFIX=i686-elf-
# Export variables
BINUTILS_BIN=$BINUTILS_DIR/bin
export INCLUDEOS_LINKER=$BINUTILS_BIN/$LINKER_PREFIX"ld"
export INCLUDEOS_ARCHIVER=$BINUTILS_BIN/$LINKER_PREFIX"ar"

if [ -e $INCLUDEOS_LINKER ]
then
    echo -e "\n>> Found linker, assuming $BINUTILS_RELEASE is already installed."
else
    ## Configure
    pushd $INCLUDEOS_BUILD/$BINUTILS_RELEASE # cd IncludeOS_build/binutils-2.25/
    echo -e "\n>> Installing binutils in $BINUTILS_DIR"
    ./configure --program-prefix=$LINKER_PREFIX --prefix=$BINUTILS_DIR --enable-multilib --enable-ld=yes --target=i686-elf --disable-werror --enable-silent-rules
    ## Install
    make -j4 --silent
    make install
    popd
fi


echo -e "\n>> Done installing Binutils."

### End Binutils ###

# Define compiler, linker and archiver
export CC=clang-3.6
export CPP=clang++-3.6
export CC_INC=$CC
export CPP_INC=$CPP
export LD=$INCLUDEOS_LINKER
export AR=$INCLUDEOS_ARCHIVER


### Install nasm # Not sure if needed yet

#echo -e "\n\n>>> Updating nasm (bootloader)" 

# Install with brew
brew install nasm

### End nasm

# STOLEN FROM install.sh
# Multitask-parameter to make
#export num_jobs=-j$((`lscpu -p | tail -1 | cut -d',' -f1` + 1 ))
#sysctl hw == lscpu on mac

echo -e "\n\n>>> Building IncludeOS"
pushd $INCLUDEOS_SRC/src
make -j
make install
popd

echo -e "\n\n>>> Compiling the vmbuilder, which makes a bootable vm out of your service"
pushd $INCLUDEOS_SRC/vmbuild
make
cp vmbuild $INCLUDEOS_HOME/
popd

echo -e "\n\n>>> Creating a virtual network, i.e. a bridge. (NOT AVAILABLE ON MAC) Ignoring..."
#sudo $INCLUDEOS_SRC/etc/create_bridge.sh

#mkdir -p $INCLUDEOS_HOME/etc
#cp $INCLUDEOS_SRC/etc/qemu-ifup $INCLUDEOS_HOME/etc/
#cp $INCLUDEOS_SRC/etc/qemu_cmd.sh $INCLUDEOS_HOME/etc/

echo -e "\n\n>>> Done! Test your installation with ./test.sh"

