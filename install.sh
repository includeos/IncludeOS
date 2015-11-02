#! /bin/bash
. ./etc/set_traps.sh

export BUILD_DIR=$HOME/IncludeOS_build
INSTALL_DIR=$BUILD_DIR/IncludeOS_install

export PREFIX=$INSTALL_DIR
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
export build_dir=$HOME/cross-dev

# Multitask-parameter to make
export num_jobs=-j4

export newlib_version=2.2.0-1

export IncludeOS_src=`pwd`
export newlib_inc=$INSTALL_DIR/i686-elf/include
export llvm_src=llvm
export llvm_build=llvm_build
export clang_version=3.6

export gcc_version=5.1.0
export binutils_version=2.25

# Options to skip steps
[ ! -v do_newlib ] && do_newlib=1
[ ! -v do_includeos ] &&  do_includeos=1
# TODO: These should be determined by inspecting if local llvm repo is up-to-date
[ ! -v install_llvm_dependencies ] &&  export install_llvm_dependencies=1
[ ! -v download_llvm ] && export download_llvm=1



# BUILDING IncludeOS
PREREQS_BUILD="build-essential make nasm texinfo clang-$clang_version clang++-$clang_version"

echo -e "\n\n >>> Trying to install prerequisites for *building* IncludeOS"
echo -e  "        Packages: $PREREQS_BUILD \n"
sudo apt-get install -y $PREREQS_BUILD

mkdir -p $BUILD_DIR
cd $BUILD_DIR

if [ ! -z $do_gcc ]; then
    echo -e "\n\n >>> GETTING / BUILDING GCC COMPILER (Required for libgcc / unwind / crt) \n"
    $IncludeOS_src/etc/cross_compiler.sh
fi

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
    sudo cp vmbuild $INSTALL_DIR/
    popd
    
    echo -e "\n >>> Building IncludeOS"
    pushd $IncludeOS_src/src
    make $num_jobs
    
    echo -e "\n >>> Installing IncludeOS"
    sudo make install
    
    echo -e "\n >>> Linking IncludeOS test-service"
    sudo make $INSTALL_DIR/crt/crti.o
    sudo make $INSTALL_DIR/crt/crtn.o
    make test
    
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
