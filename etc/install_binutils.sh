#! /bin/bash

# 1. Download the given version of GNU binutils
# 2. Extracts and configure it for our target i686-elf (triple)
# 3. Installs the tools (prefixed with the target) in /usr/local/bin
# 4. Cleans up tarball

set -e # Exit immediately on error (we're trapping the exit signal)
trap 'previous_command=$this_command; this_command=$BASH_COMMAND' DEBUG
trap 'echo -e "\nINSTALL FAILED ON COMMAND: $previous_command\n"' EXIT

TARGET="i686-elf"
BUILD_DIR=$HOME"/IncludeOS_build"
VERSION=2.27
BINUTILS="binutils-"$VERSION
TARBALL=$BINUTILS".tar.gz"
INSTALL_DIR="/usr/local"

echo -e "\n>>> Installing: $BINUTILS for $TARGET"

mkdir -p $BUILD_DIR

pushd $BUILD_DIR

echo -e "\n>> Looking for tarball ..."
if [ -e $TARBALL ]; then
    echo -e "\n> $TARBALL found."
else
    # Download binutils
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
echo -e "\n>> Configure for $TARGET to be installed in $INSTALL_DIR"
./configure --program-prefix=$TARGET- --prefix=$INSTALL_DIR --target=$TARGET --enable-multilib --enable-ld=yes --disable-werror --enable-silent-rules

echo -e "\n>> Start install"
make -j4 V=0 --silent
make install

echo -e "\n>> Installation finished"

popd

# Clean up
echo -e "\n>> Cleaning up installation ..."
rm -rf $BINUTILS $TARBALL

popd

rm -r $BUILD_DIR

echo -e "\n>>> Done installing $BINUTILS in $INSTALL_DIR"
echo -e "# Available from the following paths:"
echo -e "#    $INSTALL_DIR/$TARGET/bin"
echo -e "#    $INSTALL_DIR/bin/$TARGET-<exec>"

trap - EXIT
