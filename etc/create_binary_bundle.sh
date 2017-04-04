#! /bin/bash
. ./set_traps.sh

# Paths
export INCLUDEOS_SRC=${INCLUDEOS_SRC:-~/IncludeOS}
export BUILD_DIR=${BUILD_DIR:-~/IncludeOS_build}	# Where the libs are built
export TEMP_INSTALL_DIR=${TEMP_INSTALL_DIR:-$BUILD_DIR/IncludeOS_TEMP_install}	# Libs are installed
export PATH="$TEMP_INSTALL_DIR/bin:$PATH"

# Build options
export TARGET=i686-elf	# Configure target
export num_jobs=${num_jobs:-"-j"}	# Specify number of build jobs

# Version numbers
export binutils_version=${binutils_version:-2.28}		# ftp://ftp.gnu.org/gnu/binutils
export newlib_version=${newlib_version:-2.5.0.20170323}			# ftp://sourceware.org/pub/newlib
#export newlib_version=${newlib_version:-2.5.0}			# ftp://sourceware.org/pub/newlib
export gcc_version=${gcc_version:-6.3.0}				# ftp://ftp.nluug.nl/mirror/languages/gcc/releases/
export clang_version=${clang_version:-3.9}				# http://releases.llvm.org/
export LLVM_TAG=${LLVM_TAG:-RELEASE_381/final}			# http://llvm.org/svn/llvm-project/llvm/tags

# Options to skip steps
[ ! -v do_binutils ] && do_binutils=1
[ ! -v do_gcc ] && do_gcc=1
[ ! -v do_newlib ] && do_newlib=1
[ ! -v do_includeos ] &&  do_includeos=1
[ ! -v do_llvm ] &&  do_llvm=1
[ ! -v do_bridge ] &&  do_bridge=1

############################################################
# COMMAND LINE PROPERTIES:
############################################################

# Initialize variables:
install_yes=0

while getopts "h?y" opt; do
    case "$opt" in
    h|\?)
        printf "%s\n" "Options:"\
                "-y Yes: answer yes to install"\
        exit 0
        ;;
    y)  install_yes=1
        ;;
    esac
done

# Install build dependencies
DEPS_BUILD="build-essential make nasm texinfo clang-$clang_version clang++-$clang_version"

echo -e "\n\n >>> Trying to install prerequisites for *building* IncludeOS"
echo -e  "        Packages: $DEPS_BUILD \n"

if [ ! -z $do_packages ]; then
  sudo apt-get update
  sudo apt-get install -y $DEPS_BUILD
fi

# Print currently set install options
printf "\n\n>>> Bundle will be created with the following options:\n\n"
printf "    %-25s %-25s %s\n"\
	   "Env variable" "Description" "Value"\
	   "------------" "-----------" "-----"\
	   "INCLUDEOS_SRC" "Source dir of IncludeOS" "$INCLUDEOS_SRC"\
	   "binutils_version" "binutils version" "$binutils_version"\
	   "newlib_version" "newlib version" "$newlib_version"\
	   "gcc_version" "gcc version" "$gcc_version"\
	   "clang_version" "clang version" "$clang_version"\
	   "LLVM_TAG" "LLVM version" "$LLVM_TAG"\

# Give user option to evaluate install options
if tty -s && [ $install_yes -eq 0 ]; then
	read -p "Is this correct [Y/n]?" answer
	answer=${answer:-"Y"}	# Default value
	case $answer in
		[yY] | [yY][Ee][Ss] )
			true;;
		[nN] | [n|N][O|o] )
			exit 1;;
		*) echo "Invalid input"
		   exit 1;;
	esac
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Build all sources
if [ ! -z $do_binutils ]; then
    echo -e "\n\n >>> GETTING / BUILDING binutils (Required for libgcc / unwind / crt) \n"
    $INCLUDEOS_SRC/etc/build_binutils.sh
fi

if [ ! -z $do_gcc ]; then
    echo -e "\n\n >>> GETTING / BUILDING GCC COMPILER (Required for libgcc / unwind / crt) \n"
    $INCLUDEOS_SRC/etc/build_gcc.sh
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
DIR_NAME="IncludeOS_dependencies"
export BUNDLE_DIR=${BUNDLE_DIR:-~/$DIR_NAME}

echo ">>> Creating Installation Bundle as $BUNDLE_DIR"

OUTFILE="${DIR_NAME}_$filename_tag.tar.gz"

newlib=$TEMP_INSTALL_DIR/i686-elf/lib
llvm=$BUILD_DIR/build_llvm

# Libraries
libc=$newlib/libc.a
libm=$newlib/libm.a
libg=$newlib/libg.a
libcpp=$llvm/lib/libc++.a
libcppabi=$llvm/lib/libc++abi.a

GPP=$TEMP_INSTALL_DIR/bin/i686-elf-g++
GCC_VER=`$GPP -dumpversion`
libgcc=$TEMP_INSTALL_DIR/lib/gcc/i686-elf/$GCC_VER/libgcc.a

# Includes
include_newlib=$TEMP_INSTALL_DIR/i686-elf/include
include_libcxx=$llvm/include/c++/v1

# Make directory-tree
mkdir -p $BUNDLE_DIR
mkdir -p $BUNDLE_DIR/newlib
mkdir -p $BUNDLE_DIR/libcxx
mkdir -p $BUNDLE_DIR/crt
mkdir -p $BUNDLE_DIR/libgcc

# Copy binaries
cp $libcpp $BUNDLE_DIR/libcxx/
cp $libcppabi $BUNDLE_DIR/libcxx/
cp $libm $BUNDLE_DIR/newlib/
cp $libc $BUNDLE_DIR/newlib/
cp $libg $BUNDLE_DIR/newlib/
cp $libgcc $BUNDLE_DIR/libgcc/
cp $TEMP_INSTALL_DIR/lib/gcc/i686-elf/$GCC_VER/crt*.o $BUNDLE_DIR/crt/

# Copy includes
cp -r $include_newlib $BUNDLE_DIR/newlib/
cp -r $include_libcxx $BUNDLE_DIR/libcxx/include

# Zip it
tar -czvf $OUTFILE --directory=$BUNDLE_DIR/../ $DIR_NAME

echo ">>> IncludeOS Installation Bundle created as $INSTALL_DIR and gzipped into $OUTFILE"

trap - EXIT
