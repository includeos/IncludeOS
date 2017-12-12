#! /bin/bash
. $INCLUDEOS_SRC/etc/set_traps.sh

pushd $BUILD_DIR
MUSL_DIR="build_musl"

# Download
if [ ! -d musl-$musl_version ]; then
	echo -e "\n\n >>> Getting musl \n"
  git clone git://git.musl-libc.org/musl/ || true
fi



pushd musl
git checkout $musl_version
make distclean || true
# CC="$CC -m32"
export CFLAGS="$CFLAGS -target $ARCH-pc-linux-elf"
./configure --prefix=$TEMP_INSTALL_DIR/$TARGET  --disable-shared --enable-optimize=* --target=$TARGET
make $num_jobs
make install
