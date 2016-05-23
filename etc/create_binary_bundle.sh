#! /bin/bash

# Zip-file name
[ ! -v INCLUDEOS_SRC ] && INCLUDEOS_SRC=$HOME/IncludeOS
pushd $INCLUDEOS_SRC
tag=`git describe --abbrev=0`
filename_tag=`echo $tag | tr . -`
popd

# Where to place the installation bundle
DIR_NAME="IncludeOS_install"

[ ! -v INSTALL_DIR ] && INSTALL_DIR=$HOME/$DIR_NAME
[ ! -v BUILD_DIR ] && BUILD_DIR=$HOME/IncludeOS_build
[ ! -v TEMP_INSTALL_DIR ] && TEMP_INSTALL_DIR=$BUILD_DIR/IncludeOS_TEMP_install

echo ">>> Creating Installation Bundle as $INSTALL_DIR"

OUTFILE="${DIR_NAME}_$filename_tag.tar.gz"

newlib=$TEMP_INSTALL_DIR/i686-elf/lib
llvm=$BUILD_DIR/build_llvm

# Libraries
libc=$newlib/libc.a
libm=$newlib/libm.a
libg=$newlib/libg.a
libcpp=$llvm/lib/libc++.a

GPP=$TEMP_INSTALL_DIR/bin/i686-elf-g++
GCC_VER=`$GPP -dumpversion`
libgcc=$TEMP_INSTALL_DIR/lib/gcc/i686-elf/$GCC_VER/libgcc.a

# Includes
include_newlib=$TEMP_INSTALL_DIR/i686-elf/include
include_libcxx=$llvm/include/c++/v1

# Make directory-tree
mkdir -p $INSTALL_DIR
mkdir -p $INSTALL_DIR/newlib
mkdir -p $INSTALL_DIR/libcxx
mkdir -p $INSTALL_DIR/crt
mkdir -p $INSTALL_DIR/libgcc

# Copy binaries
cp $libcpp $INSTALL_DIR/libcxx/
cp $libm $INSTALL_DIR/newlib/
cp $libc $INSTALL_DIR/newlib/
cp $libg $INSTALL_DIR/newlib/
cp $libgcc $INSTALL_DIR/libgcc/
cp $TEMP_INSTALL_DIR/lib/gcc/i686-elf/$GCC_VER/crt*.o $INSTALL_DIR/crt/

# Copy includes
cp -r $include_newlib $INSTALL_DIR/newlib/
cp -r $include_libcxx $INSTALL_DIR/libcxx/include

# Zip it
tar -czvf $OUTFILE --directory=$INSTALL_DIR/../ $DIR_NAME

echo ">>> IncludeOS Installation Bundle created as $INSTALL_DIR and gzipped into $OUTFILE"
