#!/bin/bash
. $INCLUDEOS_SRC/etc/set_traps.sh

# Download, configure, compile and install gcc

pushd $BUILD_DIR

# Download
if [ ! -f gcc-$gcc_version.tar.gz ]; then
    echo -e "\n\n >>> Getting GCC \n"
	GCC_LOC=ftp://ftp.nluug.nl/mirror/languages/gcc/releases/
    wget -c --trust-server-name $GCC_LOC/gcc-$gcc_version/gcc-$gcc_version.tar.gz
fi

# UNPACK GCC
if [ ! -d gcc-$gcc_version ]; then
    echo -e "\n\n >>> Unpacking GCC source \n"
    tar -xf gcc-$gcc_version.tar.gz

    # GET GCC PREREQS
    echo -e "\n\n >>> Getting GCC Prerequisites \n"
    pushd gcc-$gcc_version/
    ./contrib/download_prerequisites
    popd
else
    echo -e "\n\n >>> SKIP: Unpacking GCC + getting prerequisites Seems to be there \n"
fi

# Configure
echo -e "\n\n >>> Configuring GCC \n"
if [ -d build_gcc ]; then
  echo -e "\n\n >>> Cleaning previous build \n"
  rm -rf build_gcc
fi

mkdir -p build_gcc
cd build_gcc

../gcc-$gcc_version/configure \
	--target=$TARGET \
	--prefix="$TEMP_INSTALL_DIR" \
	--disable-nls \
	--enable-languages=c,c++ \
  --without-headers

# Compile
echo -e "\n\n >>> Building GCC \n"
make all-gcc $num_jobs

# Install
echo -e "\n\n >>> Installing GCC (Might require sudo) \n"
make install-gcc

echo -e "\n\n >>> Building libgcc for target $TARGET \n"
make all-target-libgcc $num_jobs

echo -e "\n\n >>> Installing libgcc (Might require sudo) \n"
make install-target-libgcc

popd	# BUILD_DIR
trap - EXIT
