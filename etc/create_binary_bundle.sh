#! /bin/bash

[ ! -v BUILD_DIR ] && BUILD_DIR=$HOME/IncludeOS_build
[ ! -v TMP_INSTALL_DIR ] && TMP_INSTALL_DIR=$BUILD_DIR/install

newlib=$TMP_INSTALL_DIR/i686-elf/lib

FOLDER="IncludeOS_install"
OUTFILE="$FOLDER.tar.gz"

# Libraries
libc=$newlib/libc.a
libm=$newlib/libm.a
libcpp=$BUILD_DIR/llvm_build/lib/libc++.a

# Includes
include_newlib=$TMP_INSTALL_DIR/i686-elf/include
include_libcxx=$BUILD_DIR/llvm_build/include/c++/v1


# Make directory-tree
mkdir -p $FOLDER
mkdir -p $FOLDER/newlib
mkdir -p $FOLDER/libcxx
mkdir -p $FOLDER/crt

# Copy binaries
cp $libcpp $FOLDER/libcxx/
cp $libm $FOLDER/newlib/
cp $libc $FOLDER/newlib/
cp $BUILD_DIR/crt/*.o $FOLDER/crt/

# Copy includes
cp -r $include_newlib $FOLDER/newlib/
cp -r $include_libcxx $FOLDER/libcxx/include

# Zip it
tar -czvf $OUTFILE $FOLDER

echo ">>> Bundle created as $FOLDER and gzipped into $OUTFILE"
