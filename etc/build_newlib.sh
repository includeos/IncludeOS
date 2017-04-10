#! /bin/bash
. $INCLUDEOS_SRC/etc/set_traps.sh

# Download, configure, compile and install newlib

pushd $BUILD_DIR
NEWLIB_DIR="build_newlib"

# Download
if [ ! -d newlib-$newlib_version ]; then
    if [ ! -f newlib-$newlib_version.tar.gz ]; then
		echo -e "\n\n >>> Getting newlib \n"
		wget -c --trust-server-name ftp://sourceware.org/pub/newlib/newlib-$newlib_version.tar.gz
    else
		echo -e "\n\n >>> SKIP: Download newlib. Found tarball "$newlib_version.tar.gz" \n"
    fi
    echo -e "\n\n >>> Extracting newlib \n"
    tar -xf newlib-$newlib_version.tar.gz

else
    echo -e "\n\n >>> SKIP:  Download / extract newlib. Found source folder "newlib-$newlib_version" \n"
fi

# Old Note: Clean out config cache in case the cross-compiler has changed: make distclean

# Configure
echo -e "\n\n >>> Configuring newlib \n"
mkdir -p build_newlib
pushd build_newlib

../newlib-$newlib_version/configure \
	--target=$TARGET \
	--prefix=$TEMP_INSTALL_DIR \
	--enable-newlib-io-long-long AS_FOR_TARGET=as LD_FOR_TARGET=ld AR_FOR_TARGET=ar RANLIB_FOR_TARGET=ranlib \
  --enable-newlib-hw-fp \
  --enable-newlib-mb \
  --enable-newlib-iconv \
  --enable-newlib-iconv-encodings=utf-16,utf-8,ucs_2 \
  --disable-libgloss \
  --disable-multilib \
  --enable-newlib-multithread \
  --disable-newlib-supplied-syscalls

echo -e "\n\n >>> BUILDING NEWLIB \n\n"
# Compile
make $num_jobs all
# Install
make install

popd # build_newlib
popd # BUILD_DIR

trap - EXIT
