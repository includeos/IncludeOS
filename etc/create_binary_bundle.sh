#! /bin/bash
. ./set_traps.sh

# Env variables
export INCLUDEOS_SRC=${INCLUDEOS_SRC:-~/IncludeOS}
export BUILD_DIR=${BUILD_DIR:-~/IncludeOS_build}
export TEMP_INSTALL_DIR=${TEMP_INSTALL_DIR:-$BUILD_DIR/IncludeOS_TEMP_install}

export TARGET=i686-elf	# Configure target
export PREFIX=$TEMP_INSTALL_DIR
export PATH="$PREFIX/bin:$PATH"

# Build_llvm specific options
export newlib_inc=$TEMP_INSTALL_DIR/i686-elf/include
export llvm_src=llvm
export llvm_build=build_llvm
# TODO: These should be determined by inspecting if local llvm repo is up-to-date
[ ! -v install_llvm_dependencies ] &&  export install_llvm_dependencies=1
[ ! -v download_llvm ] && export download_llvm=1

export binutils_version=${binutils_version:-2.28}		# ftp://ftp.gnu.org/gnu/binutils
export newlib_version=${newlib_version:-2.5.0}			# ftp://sourceware.org/pub/newlib
export gcc_version=${gcc_version:-6.3.0}				# ftp://ftp.nluug.nl/mirror/languages/gcc/releases/
export clang_version=${clang_version:-3.9}				# http://releases.llvm.org/
export LLVM_TAG=${LLVM_TAG:-RELEASE_391/final}			# http://llvm.org/svn/llvm-project/llvm/tags

export libcpp_version=${libcpp_version:-3.9.1}			# Not in use anywhere???

# Options to skip steps
[ ! -v do_binutils ] && do_binutils=1
[ ! -v do_gcc ] && do_gcc=1
[ ! -v do_newlib ] && do_newlib=1
[ ! -v do_includeos ] &&  do_includeos=1
[ ! -v do_llvm ] &&  do_llvm=1
[ ! -v do_bridge ] &&  do_bridge=1

# Install build dependencies
DEPS_BUILD="build-essential make nasm texinfo clang-$clang_version clang++-$clang_version"

echo -e "\n\n >>> Trying to install prerequisites for *building* IncludeOS"
echo -e  "        Packages: $DEPS_BUILD \n"
sudo apt-get update
sudo apt-get install -y $DEPS_BUILD

mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Build all sources
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

#
# Create the actual bundle
#
# Zip-file name
pushd $INCLUDEOS_SRC
tag=`git describe --abbrev=0`
filename_tag=`echo $tag | tr . -`
popd

# Where to place the installation bundle
DIR_NAME="IncludeOS_install"
export INSTALL_DIR=${INSTALL_DIR:-~/$DIR_NAME}

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

trap - EXIT
