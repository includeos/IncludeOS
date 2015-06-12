export PREFIX="/usr/local/IncludeOS"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

binutils_version=2.25
gcc_version=5.1.0
installdir=$HOME/cross-dev

mkdir -p $installdir
cd $installdir

echo -e "\n\n >>> Getting binutils"
wget -c --trust-server-name ftp://ftp.uninett.no/pub/gnu/binutils/binutils-$binutils_version.tar.gz

echo -e "\n\n >>> Getting GCC"
wget -c --trust-server-name ftp://ftp.uninett.no/pub/gnu/gcc/gcc-$gcc_version/gcc-$gcc_version.tar.gz

echo -e "\n\n >>> Extracting binutils \n"
tar -xf binutils-$binutils_version.tar.gz

echo -e "\n\n >>> Building binutils"
mkdir -p build-binutils
cd build-binutils
../binutils-$binutils_version/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --disable-werror
make -j8
make install

# UNPACK GCC
echo -e "\n\n >>> Getting GCC Prerequisites"
cd $installdir
tar -xf gcc-$gcc_version.tar.gz

# GET GCC PREREQS 
echo -e "\n\n >>> Getting GCC Prerequisites"
cd gcc-$gcc_version/
./contrib/download_prerequisites


cd $installdir
echo -e "\n\n >>> Building GCC"
mkdir -p build-gcc

cd build-gcc
../gcc-$gcc_version/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers

make all-gcc -j8
make all-target-libgcc -j8

echo -e "\n\n >>> Installing GCC"
make install-gcc
make install-target-libgcc
