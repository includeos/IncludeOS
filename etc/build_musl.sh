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

# Replace syscall API
cp $INCLUDEOS_SRC/api/syscalls.h src/internal/includeos_syscalls.h
cp $INCLUDEOS_SRC/etc/musl/syscall.h src/internal/
rm -f arch/x86_64/syscall_arch.h
rm -f arch/i386/syscall_arch.h

# Compatibility patch
git apply $INCLUDEOS_SRC/etc/musl/musl.patch || true
git apply $INCLUDEOS_SRC/etc/musl/endian.patch || true


export CFLAGS="$CFLAGS -g -target $ARCH-pc-linux-elf"
./configure --prefix=$TEMP_INSTALL_DIR/$TARGET  --disable-shared --enable-debug --target=$TARGET #--enable-optimize=*
make $num_jobs
make install


trap - EXIT
