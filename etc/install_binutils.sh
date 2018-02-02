#! /bin/bash

# 1. Download the given version of GNU binutils
# 2. Extracts and configure it for our target i686-elf (triple)
# 3. Installs the tools (prefixed with the target) in /usr/local/bin
# 4. Cleans up tarball


export INCLUDEOS_SRC=${INCLUDEOS_SRC:-"~/IncludeOS"}
export INCLUDEOS_PREFIX=${INCLUDEOS_PREFIX/:-"/usr/local"}
export BUILD_DIR="/tmp/IncludeOS_build"
TMP_INSTALL_DIR="/tmp/IncludeOS_binutils_install"
INSTALL_DIR=$INCLUDEOS_PREFIX/includeos/bin
export ARCH=${ARCH:-x86_64} # CPU architecture. Alternatively i686
export TARGET=$ARCH-elf	# Configure target based on arch. Always ELF.
export PROGRAM_PREFIX=$ARCH-pc-linux-elf	# Configure target based on arch. Always ELF.
VERSION=2.29.1
BINUTILS="binutils-"$VERSION
TARBALL=$BINUTILS".tar.gz"

. $INCLUDEOS_SRC/etc/set_traps.sh
echo -e "\n>>> Installing: $BINUTILS for $TARGET"

mkdir -p $BUILD_DIR
pushd $BUILD_DIR

# Download binutils if needed
echo -e "\n>> Looking for tarball ..."
if [ -e $TARBALL ]; then
    echo -e "\n> $TARBALL found."
else
    echo -e "\n> Downloading $TARBALL ..."
    curl https://ftp.gnu.org/gnu/binutils/$TARBALL -o $TARBALL
fi

# Unzip
if [ ! -d $BINUTILS ]; then
    echo -e "\n>> Unzipping $TARBALL ..."
    tar -xzf $TARBALL -C .
fi

pushd $BINUTILS

# Configure & install
mkdir -p $TMP_INSTALL_DIR
echo -e "\n>> Configure for $TARGET to be installed in $TMP_INSTALL_DIR"
./configure --program-prefix=$PROGRAM_PREFIX- --prefix=$TMP_INSTALL_DIR --target=$TARGET --enable-multilib --enable-ld=yes --disable-werror --enable-silent-rules

echo -e "\n>> Start install"
make -j4 V=0 --silent
make install
echo -e "\n>> Installation finished"

# Copy binaries to proper location
mkdir -p $INSTALL_DIR
cp $TMP_INSTALL_DIR/bin/* $INSTALL_DIR

# Clean up
popd	# Out of $BINUTILS
echo -e "\n>> Cleaning up installation ..."
rm -rf $BINUTILS $TARBALL
popd	# Out of $BUILD_DIR
rm -r $BUILD_DIR
rm -r $TMP_INSTALL_DIR

echo -e "\n>>> Done installing $BINUTILS in $INSTALL_DIR"
echo -e "# Available from the following paths:"
echo -e "#    $INSTALL_DIR/$TARGET/bin"
echo -e "#    $INSTALL_DIR/bin/$TARGET-<exec>"

trap - EXIT
