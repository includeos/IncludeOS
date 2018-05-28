#! /bin/bash
#. $INCLUDEOS_SRC/etc/set_traps.sh

# Dependencies
sudo apt install autoconf

# Download, configure, compile and install llvm
ARCH=${ARCH:-x86_64} # CPU architecture. Alternatively x86_64
TARGET=$ARCH-elf	# Configure target based on arch. Always ELF.
BUILD_DIR=${BUILD_DIR:-~/IncludeOS_build/build_llvm}

# Download
if [ ! -d libunwind ]; then
	echo -e "\n\n >>> Getting libunwind \n"
  git clone git://git.sv.gnu.org/libunwind.git
fi

cd libunwind

git checkout $libunwind_version
make clean || true

ls -l

./autogen.sh

# NOTE: For some reason target x86_pc-elf doesn't work for libunwind.
./configure --prefix=$TEMP_INSTALL_DIR/$TARGET  --disable-shared --enable-debug --target=$ARCH-linux --enable-cxx-exceptions
make $num_jobs
make install


#trap - EXIT
