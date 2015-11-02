#! /bin/bash

[ ! -v BUILD_DIR ] && BUILD_DIR=$HOME/IncludeOS_build
[ ! -v INSTALL_DIR ] && INSTALL_DIR=$BUILD_DIR/IncludeOS_install

newlib=$INSTALL_DIR/i686-elf/lib
llvm=$BUILD_DIR/build_llvm

FOLDER="IncludeOS_bundle"
OUTFILE="$FOLDER.tar.gz"

# Libraries
libc=$newlib/libc.a
libm=$newlib/libm.a
libg=$newlib/libg.a
libcpp=$llvm/lib/libc++.a

GPP=$INSTALL_DIR/bin/i686-elf-g++
GCC_VER=`$GPP -dumpversion`
libgcc=$INSTALL_DIR/lib/gcc/i686-elf/$GCC_VER/libgcc.a

# Includes
include_newlib=$INSTALL_DIR/i686-elf/include
include_libcxx=$llvm/include/c++/v1



# Make directory-tree
mkdir -p $FOLDER
mkdir -p $FOLDER/newlib
mkdir -p $FOLDER/libcxx
mkdir -p $FOLDER/crt
mkdir -p $FOLDER/libgcc

# Copy binaries
cp $libcpp $FOLDER/libcxx/
cp $libm $FOLDER/newlib/
cp $libc $FOLDER/newlib/
cp $libg $FOLDER/newlib/
cp $libgcc $FOLDER/libgcc/
cp $INSTALL_DIR/lib/gcc/i686-elf/$GCC_VER/crt*.o $FOLDER/crt/

# Copy includes
cp -r $include_newlib $FOLDER/newlib/
cp -r $include_libcxx $FOLDER/libcxx/include

# Zip it
tar -czvf $OUTFILE $FOLDER

echo ">>> IncludeOS Installation Bundle created as $FOLDER and gzipped into $OUTFILE"
