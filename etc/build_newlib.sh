#! /bin/bash
. $INCLUDEOS_SRC/etc/set_traps.sh


# Configure for an "unspecified x86 elf" target, 

pushd $BUILD_DIR
NEWLIB_DIR="build_newlib"

if [ ! -d newlib-$newlib_version ]; then
    
    if [ ! -f newlib-$newlib_version.tar.gz ]; then
	echo -e "\n\n >>> Getting newlib \n"
	wget -c --trust-server-name ftp://sourceware.org/pub/newlib/newlib-$newlib_version.tar.gz
    else
	echo -e "\n\n >>> SKIP: Download newlib. Fonund tarball "$newlib_version.tar.gz" \n"
    fi
    echo -e "\n\n >>> Extracting newlib \n"
    tar -xf newlib-$newlib_version.tar.gz


    # PATCH newlib, to be compatible with clang.
    echo -e "\n\n >>> Patching newlib, to build with clang \n"
    patch -p0 < $INCLUDEOS_SRC/etc/newlib_clang.patch

else
    echo -e "\n\n >>> SKIP:  Download / extract newlib. Found source folder "newlib-$newlib_version" \n"
fi

echo -e "\n\n >>> Configuring newlib \n"
mkdir -p $NEWLIB_DIR
pushd $NEWLIB_DIR

# Clean out config cache in case the cross-compiler has changed
# make distclean
../newlib-$newlib_version/configure --target=$TARGET --prefix=$PREFIX --enable-newlib-io-long-long AS_FOR_TARGET=as LD_FOR_TARGET=ld AR_FOR_TARGET=ar RANLIB_FOR_TARGET=ranlib #CC_FOR_TARGET="clang-3.6 -Wno-return-type --target=i686-pc-none-elf" #--target=i686-elf -ffreestanding

echo -e "\n\n >>> BUILDING NEWLIB \n\n"    
make $num_jobs all 
make install

popd # NEWLIB_DIR
popd # BUILD_DIR

trap - EXIT
