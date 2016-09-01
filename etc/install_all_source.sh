#! /bin/bash
. ./etc/set_traps.sh

export BUILD_DIR=$HOME/IncludeOS_build
export TEMP_INSTALL_DIR=$BUILD_DIR/IncludeOS_TEMP_install

export INSTALL_DIR=$HOME/IncludeOS_install
export PREFIX=$TEMP_INSTALL_DIR
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
export build_dir=$HOME/cross-dev

export newlib_version=2.4.0

export INCLUDEOS_SRC=`pwd`
export newlib_inc=$TEMP_INSTALL_DIR/i686-elf/include
export llvm_src=llvm
export llvm_build=build_llvm
export clang_version=3.8
export libcpp_version=3.8.1

export gcc_version=6.2.0
export binutils_version=2.26

# Options to skip steps
[ ! -v do_binutils ] && do_binutils=1
[ ! -v do_gcc ] && do_gcc=1
[ ! -v do_newlib ] && do_newlib=1
[ ! -v do_includeos ] &&  do_includeos=1
[ ! -v do_llvm ] &&  do_llvm=1
# TODO: These should be determined by inspecting if local llvm repo is up-to-date

[ ! -v install_llvm_dependencies ] &&  export install_llvm_dependencies=1
[ ! -v download_llvm ] && export download_llvm=1

#. $INCLUDEOS_SRC/etc/prepare_ubuntu_deps.sh

# BUILDING IncludeOS
DEPS_BUILD="build-essential make nasm texinfo clang-$clang_version clang++-$clang_version"


echo -e "\n\n >>> Trying to install prerequisites for *building* IncludeOS"
echo -e  "        Packages: $DEPS_BUILD \n"
sudo apt-get install -y $DEPS_BUILD

mkdir -p $BUILD_DIR
cd $BUILD_DIR

if [ ! -z $do_binutils ]; then
    echo -e "\n\n >>> GETTING / BUILDING binutils (Required for libgcc / unwind / crt) \n"
    $INCLUDEOS_SRC/etc/build_binutils.sh
fi

if [ ! -z $do_gcc ]; then
    echo -e "\n\n >>> GETTING / BUILDING GCC COMPILER (Required for libgcc / unwind / crt) \n"
    $INCLUDEOS_SRC/etc/cross_compiler.sh
fi

if [ ! -z $do_newlib ]; then
    echo -e "\n\n >>> GETTING / BUILDING NEWLIB \n"
    $INCLUDEOS_SRC/etc/build_newlib.sh
fi

if [ ! -z $do_llvm ]; then
    echo -e "\n\n >>> GETTING / BUILDING llvm / libc++ \n"
    $INCLUDEOS_SRC/etc/build_llvm32.sh
fi

echo -e "\n >>> DEPENDENCIES SUCCESSFULLY BUILT. Creating binary bundle \n"
$INCLUDEOS_SRC/etc/create_binary_bundle.sh

echo -e "\n\n>>> Installing submodules"
pushd $INCLUDEOS_SRC
git submodule init
git submodule update
popd

if [ ! -z $do_includeos ]; then
    # Install OS before vmbuilder
    echo -e "\n >>> Building IncludeOS"
    pushd $INCLUDEOS_SRC/src
    make clean
    make -j

    echo -e "\n >>> Installing IncludeOS"
    make install
    popd

    # Build and install the vmbuilder (after OS)
    echo -e "\n >>> Installing vmbuilder"
    pushd $INCLUDEOS_SRC/vmbuild
    make
    cp vmbuild $INSTALL_DIR/
    popd

    # RUNNING IncludeOS
    DEPS_RUN="bridge-utils qemu-kvm"
    echo -e "\n\n >>> Trying to install prerequisites for *running* IncludeOS"
    echo -e   "        Packages: $DEPS_RUN \n"
    sudo apt-get install -y $DEPS_RUN

    # Set up the IncludeOS network bridge
    echo -e "\n\n >>> Create IncludeOS network bridge  *Requires sudo* \n"
    sudo $INCLUDEOS_SRC/etc/create_bridge.sh

    # Copy qemu-ifup til install loc.
    $INCLUDEOS_SRC/etc/copy_scripts.sh
fi

echo -e "\n >>> Done. Test the installation by running ./test.sh \n"

trap - EXIT
