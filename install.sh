#! /bin/bash


INSTALL_DIR=/usr/local/IncludeOS/
export BUILD_DIR=$HOME/IncludeOS_build

export PREFIX=$INSTALL_DIR
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
export build_dir=$HOME/cross-dev

# Multitask-parameter to make
export num_jobs=-j8

export binutils_version=2.25
export gcc_version=5.1.0
export newlib_version=2.2.0-1


export IncludeOS_src=`pwd`
export newlib_inc=$INSTALL_DIR/i686-elf/include
export llvm_src=llvm
export llvm_build=llvm_build

if [ ! -v do_newlib ]; then
    do_newlib=1
fi

if [ ! -v do_includeos ]; then
    do_includeos=1
fi

# TODO: These should be determined by inspecting if local llvm repo is up-to-date
if [ ! -v install_llvm_dependencies ]; then
    export install_llvm_dependencies=1
fi

if [ ! -v download_llvm ]; then
    export download_llvm=1
fi


# Utilities
. ./etc/bash_functions.sh


# TODO: Implement checks to see which steps can be skipped!


# BUILDING IncludeOS

PREREQS_BUILD="gcc g++ build-essential make nasm texinfo"

# Get prerequisite packages, such as a compiler, GNU Make, etc.
echo -e "\n\n >>> Trying to install prerequisites for *building* IncludeOS"
echo -e  "        Packages: $PREREQS_BUILD \n"
sudo apt-get install -y $PREREQS_BUILD
or_die "Couldn't install required packages"


#
# DEPRECATED: We're building with clang now
#
# Get and build the actual toolchain
# echo -e "\n\n >>> GETTING / BUILDING CROSS COMPILER \n"
# ./etc/cross_compiler.sh
# or_die "Couldn't install cross compiler"

mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo -e "\n\n >>> GETTING / BUILDING llvm / libc++ \n"
$IncludeOS_src/etc/build_llvm32.sh


if [ ! -z $do_newlib ]; then
    echo -e "\n\n >>> GETTING / BUILDING NEWLIB \n"
    $IncludeOS_src/etc/build_newlib.sh
    or_die "Couldn't install newlib"
fi

if [ ! -z $do_includeos ]; then
    # Build and install the vmbuilder 
    echo -e "\n >>> Installing vmbuilder"
    pushd $IncludeOS_src/vmbuild
    make
    sudo cp vmbuild $OSDIR/
    or_die "Couldn't install vmbuilder"
    popd
    
    echo -e "\n >>> Building IncludeOS"
    pushd $IncludeOS_src/src
    make 
    or_die "Couldn't build IncludeOS"
    
    echo -e "\n >>> Installing IncludeOS"
    sudo make install
    or_die "Couldn't install IncludeOS"
    popd
    
    # RUNNING IncludeOS
    PREREQS_RUN="bridge-utils qemu-kvm"
    echo -e "\n\n >>> Trying to install prerequisites for *running* IncludeOS"
    echo -e   "        Packages: $PREREQS_RUN \n"
    sudo apt-get install -y $PREREQS_RUN
    or_die "Couldn't install packages needed to run IncludeOS"
    
    # Set up the IncludeOS network bridge
    echo -e "\n\n >>> Create IncludeOS network bridge "`pwd`"\n"
    sudo $IncludeOS_src/etc/create_bridge.sh
    sudo cp $IncludeOS_src/etc/qemu-ifup /etc
    or_die "Couldn't set up the network bridge"
fi

echo -e "\n >>> Done. Test the installation by running ./test.sh \n"

