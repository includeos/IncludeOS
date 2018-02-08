#! /bin/bash
. $INCLUDEOS_SRC/etc/set_traps.sh

# Paths
export INCLUDEOS_SRC=${INCLUDEOS_SRC:-~/IncludeOS}
export BUILD_DIR=${BUILD_DIR:-~/IncludeOS_build}	# Where the libs are built
export TEMP_INSTALL_DIR=${TEMP_INSTALL_DIR:-$BUILD_DIR/IncludeOS_TEMP_install}	# Libs are installed
export PATH="$TEMP_INSTALL_DIR/bin:$PATH"

# Build options
export ARCH=${ARCH:-i686} # CPU architecture. Alternatively x86_64
export BUNDLE_ARCHES=${BUNDLE_ARCHES:-"i686 x86_64"}
export TARGET=$ARCH-elf	# Configure target based on arch. Always ELF.
export num_jobs=${num_jobs:--j}	# Specify number of build jobs

# Version numbers
export binutils_version=${binutils_version:-2.29.1}		# ftp://ftp.gnu.org/gnu/binutils
export newlib_version=${newlib_version:-2.5.0.20170922}			# ftp://sourceware.org/pub/newlib
export gcc_version=${gcc_version:-7.2.0}				# ftp://ftp.nluug.nl/mirror/languages/gcc/releases/
export clang_version=${clang_version:-4.0}				# http://releases.llvm.org/
export llvm_branch=${llvm_branch:-release_50}

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

# NOTE: Addding  llvm 5.0 package sources, needed for Ubuntu 16.04 LTS can be done like so:
# llvm_source_list=/etc/apt/sources.list.d/llvm.list
# llvm_deb_entry="deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-5.0 main"
# sudo grep -q -F "$llvm_deb_entry" $llvm_source_list || sudo bash -c "echo \"$llvm_deb_entry\" >> $llvm_source_list"
# wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -

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
	   "LLVM_branch" "LLVM version" "$llvm_branch"\
     "BUNDLE_ARCHES" "Build for CPU arches" "$BUNDLE_ARCHES"\

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


pushd $INCLUDEOS_SRC
tag=`git describe --abbrev=0`
filename_tag=`echo $tag | tr . -`
popd

# Where to place the installation bundle
DIR_NAME="IncludeOS_dependencies"
OUTFILE="${DIR_NAME}_$filename_tag.tar.gz"
BUNDLE_PATH=${BUNDLE_PATH:-~}

function do_build {
  echo -e "\n\n >>> Building bundle for ${ARCH} \n"
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
    $INCLUDEOS_SRC/etc/build_llvm.sh
  fi

  #
  # Create the actual bundle
  #


  BUNDLE_DIR=$BUNDLE_PATH/$DIR_NAME
  BUNDLE_LOC=${BUNDLE_DIR}/$ARCH

  echo ">>> Creating Installation Bundle as $BUNDLE_LOC"

  newlib=$TEMP_INSTALL_DIR/$TARGET/lib
  llvm=$BUILD_DIR/build_llvm

  # Libraries
  libc=$newlib/libc.a
  libm=$newlib/libm.a
  libg=$newlib/libg.a
  libcpp=$llvm/lib/libc++.a
  libcppabi=$llvm/lib/libc++abi.a

  GPP=$TEMP_INSTALL_DIR/bin/$TARGET-g++
  GCC_VER=`$GPP -dumpversion`
  libgcc=$TEMP_INSTALL_DIR/lib/gcc/$TARGET/$GCC_VER/libgcc.a

  # Includes
  include_newlib=$TEMP_INSTALL_DIR/$TARGET/include
  include_libcxx=$llvm/include/c++/v1

  # Make directory-tree
  mkdir -p $BUNDLE_LOC
  mkdir -p $BUNDLE_LOC/newlib
  mkdir -p $BUNDLE_LOC/libcxx
  mkdir -p $BUNDLE_LOC/crt
  mkdir -p $BUNDLE_LOC/libgcc

  # Copy binaries
  cp $libcpp $BUNDLE_LOC/libcxx/
  cp $libcppabi $BUNDLE_LOC/libcxx/
  cp $libm $BUNDLE_LOC/newlib/
  cp $libc $BUNDLE_LOC/newlib/
  cp $libg $BUNDLE_LOC/newlib/
  cp $libgcc $BUNDLE_LOC/libgcc/
  cp $TEMP_INSTALL_DIR/lib/gcc/$TARGET/$GCC_VER/crt*.o $BUNDLE_LOC/crt/

  # Copy includes
  cp -r $include_newlib $BUNDLE_LOC/newlib/
  cp -r $include_libcxx $BUNDLE_LOC/libcxx/include
}

for B_ARCH in $BUNDLE_ARCHES
do
  export ARCH=$B_ARCH
  export TARGET=$ARCH-elf	# Configure target based on arch. Always ELF.
  do_build
done

# Zip it
tar -czvf $OUTFILE --directory=$BUNDLE_PATH $DIR_NAME

echo ">>> IncludeOS Installation Bundle created as $BUNDLE_PATH/$OUTFILE"

trap - EXIT
