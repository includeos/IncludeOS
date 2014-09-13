export PREFIX="/usr/local/IncludeOS"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

binutils_version=2.24
gcc_version=4.9.1
gmp_version=6.0.0 # NOTE: There might be a sub-version letter for the .gz file, but not for the folder name
mpfr_version=3.1.2
isl_version=0.13
cloog_version=0.18.1
mpc_version=1.0.2

cd $HOME/src

echo -e "\n\n Getting binutils"
wget -c --trust-server-name http://ftp.gnu.org/gnu/binutils/binutils-2.24.tar.gz

echo -e "\n\n Getting GCC"
wget -c --trust-server-name ftp://ftp.nluug.nl/mirror/languages/gcc/releases/gcc-4.9.1/gcc-4.9.1.tar.gz


echo -e "\n\n Getting GMP"
wget -c --trust-server-name https://gmplib.org/download/gmp/gmp-6.0.0a.tar.lz

echo -e "\n\n Getting MPFR"
wget -c --trust-server-name http://www.mpfr.org/mpfr-current/mpfr-3.1.2.tar.xz

echo -e "\n\n Getting ISL"
wget -c --trust-server-name http://isl.gforge.inria.fr/isl-0.13.tar.gz

echo -e "\n\n Getting Gloog"
wget -c --trust-server-name http://www.bastoul.net/cloog/pages/download/count.php3?url=./cloog-0.18.1.tar.gz

echo -e "\n\n Getting MPC"
wget -c --trust-server-name ftp://ftp.gnu.org/gnu/mpc/mpc-1.0.2.tar.gz


echo -e "\n\n Extracting ...\n"
tar -xf binutils-$binutils_version.tar.gz

# If you wish to build these packages as part of gcc:
tar -xf gcc-$gcc_version.tar.gz
tar -xf gmp-"$gmp_version"a.tar.lz #OBS: There's a sub-version letter here
tar -xf mpfr-$mpfr_version.tar.xz
tar -xf mpc-$mpc_version.tar.gz
tar -xf cloog-$cloog_version.tar.gz


echo -e "\n\n Building binutils"
mkdir -p build-binutils
cd build-binutils
../binutils-$binutils_version/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --disable-werror
make
make install

cd $HOME/src

sudo apt-get install -y lzip
 
mv gmp-$gmp_version gcc-$gcc_version/gmp
mv mpfr-$mpfr_version gcc-$gcc_version/mpfr
mv mpc-$mpc_version gcc-$gcc_version/mpc
mv cloog-$cloog_version gcc-$gcc_version/gloog

mkdir build-gcc
cd build-gcc
../gcc-$gcc_version/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers

make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
