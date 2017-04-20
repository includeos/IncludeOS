#! /bin/bash
. $INCLUDEOS_SRC/etc/set_traps.sh

# Download, configure and build binutils

pushd $BUILD_DIR

# Downloading
if [ ! -f binutils-$binutils_version.tar.gz ]; then
    echo -e "\n\n >>> Getting binutils into `pwd` \n"
    wget -c --trust-server-name ftp://ftp.gnu.org/gnu/binutils/binutils-$binutils_version.tar.gz
fi

# Extracting
if [ ! -d binutils-$binutils_version ]; then
    echo -e "\n\n >>> Extracting binutils \n"
    tar -xf binutils-$binutils_version.tar.gz
	rm -rf build_binutils	# If a new version has been downloaded it will be built
else
    echo -e "\n\n >>> SKIP: Extracting binutils  \n"
fi

# Configuring
echo -e "\n\n >>> Configuring binutils \n"

if [ -d build_binutils ]; then

  # We don't know if the previous build was for a different target so remove
  echo -e "\n\n >>> Cleaning previous build \n"
  rm -rf build_binutils
fi

mkdir -p build_binutils
cd build_binutils


../binutils-$binutils_version/configure \
	--target=$TARGET \
	--prefix="$TEMP_INSTALL_DIR" \
	--disable-nls \
	--disable-werror

# Compiling
    echo -e "\n\n >>> Building binutils \n"
    make $num_jobs

# Installing
    echo -e "\n\n >>> Installing binutils \n"
    make install

popd

trap - EXIT
