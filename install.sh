#! /bin/bash
. ./etc/set_traps.sh

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

if [ ! -v do_gcc ]; then
    do_gcc=1
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



# TODO: Implement checks to see which steps can be skipped!


# BUILDING IncludeOS

PREREQS_BUILD="gcc g++ build-essential make nasm texinfo"

# Get prerequisite packages, such as a compiler, GNU Make, etc.
echo -e "\n\n >>> Trying to install prerequisites for *building* IncludeOS"
echo -e  "        Packages: $PREREQS_BUILD \n"
sudo apt-get install -y $PREREQS_BUILD


#
# DEPRECATED: We're building with clang now. Keeping it until newlib can be built with clang
#
# Get and build the actual toolchain
if [ ! -z $do_gcc ]; then
    echo -e "\n\n >>> GETTING / BUILDING CROSS COMPILER \n"
    ./etc/cross_compiler.sh
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR

if [ ! -z $do_newlib ]; then
    echo -e "\n\n >>> GETTING / BUILDING NEWLIB \n"
    $IncludeOS_src/etc/build_newlib.sh
fi


echo -e "\n\n >>> GETTING / BUILDING llvm / libc++ \n"
$IncludeOS_src/etc/build_llvm32.sh

echo -e "\n\n >>> INSTALLING libc++ \n"
sudo cp $BUILD_DIR/$llvm_build/lib/libc++.a $INSTALL_DIR/lib/


if [ ! -z $do_includeos ]; then
    # Build and install the vmbuilder 
    echo -e "\n >>> Installing vmbuilder"
    pushd $IncludeOS_src/vmbuild
    make
    sudo cp vmbuild $OSDIR/
    popd
    
    echo -e "\n >>> Building IncludeOS"
    pushd $IncludeOS_src/src
    make 
    
    echo -e "\n >>> Installing IncludeOS"
    sudo make install
    popd
    
    # RUNNING IncludeOS
    PREREQS_RUN="bridge-utils qemu-kvm"
    echo -e "\n\n >>> Trying to install prerequisites for *running* IncludeOS"
    echo -e   "        Packages: $PREREQS_RUN \n"
    sudo apt-get install -y $PREREQS_RUN
    
    # Set up the IncludeOS network bridge
    echo -e "\n\n >>> Create IncludeOS network bridge "`pwd`"\n"
    sudo $IncludeOS_src/etc/create_bridge.sh
    sudo cp $IncludeOS_src/etc/qemu-ifup /etc
fi

echo -e "\n >>> Done. Test the installation by running ./test.sh \n"

trap - EXIT
