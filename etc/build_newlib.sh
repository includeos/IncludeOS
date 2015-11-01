#! /bin/bash
. $IncludeOS_src/etc/set_traps.sh


# Configure for an "unspecified x86 elf" target, 

cd $BUILD_DIR
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
else
    echo -e "\n\n >>> SKIP:  Download / extract newlib. Found source folder "newlib-$newlib_version" \n"
fi

# PATCH newlib, to be compatible with clang.
echo -e "\n\n >>> Patching newlib, to build with clang \n"
patch -p0 < $IncludeOS_src/etc/newlib_clang.patch

echo -e "\n\n >>> Configuring newlib \n"
mkdir -p $NEWLIB_DIR
pushd build_newlib

# Clean out config cache in case the cross-compiler has changed
# make distclean
../newlib-$newlib_version/configure --target=$TARGET --prefix=$PREFIX --enable-newlib-io-long-long CC_FOR_TARGET="clang-3.6 -ffreestanding --target=i686-elf -Wno-return-type" AS_FOR_TARGET=as LD_FOR_TARGET=ld AR_FOR_TARGET=ar RANLIB_FOR_TARGET=ranlib

echo -e "\n\n >>> BUILDING NEWLIB \n\n"    
make $num_jobs all 

popd

trap - EXIT
