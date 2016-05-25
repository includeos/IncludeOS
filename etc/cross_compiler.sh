# Bash utils
. $INCLUDEOS_SRC/etc/set_traps.sh

mkdir -p $BUILD_DIR
cd $BUILD_DIR

GCC_LOC=ftp://ftp.nluug.nl/mirror/languages/gcc/releases/

if [ ! -f gcc-$gcc_version.tar.gz ]; then
    echo -e "\n\n >>> Getting GCC \n"
    wget -c --trust-server-name $GCC_LOC/gcc-$gcc_version/gcc-$gcc_version.tar.gz
fi

# UNPACK GCC
if [ ! -d gcc-$gcc_version ]; then
    echo -e "\n\n >>> Unpacking GCC source \n"
    cd $BUILD_DIR
    tar -xf gcc-$gcc_version.tar.gz

    # GET GCC PREREQS 
    echo -e "\n\n >>> Getting GCC Prerequisites \n"
    pushd gcc-$gcc_version/    
    ./contrib/download_prerequisites
    popd
else
    echo -e "\n\n >>> SKIP: Unpacking GCC + getting prerequisites Seems to be there \n"
fi

cd $BUILD_DIR


mkdir -p build_gcc
cd build_gcc

echo -e "\n\n >>> Configuring GCC \n"
../gcc-$gcc_version/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers

echo -e "\n\n >>> Building GCC \n"
make all-gcc $num_jobs

echo -e "\n\n >>> Installing GCC (Might require sudo) \n"
make install-gcc

echo -e "\n\n >>> Building libgcc for target $TARGET \n"
make all-target-libgcc $num_jobs

echo -e "\n\n >>> Installing libgcc (Might require sudo) \n"
make install-target-libgcc


trap - EXIT
