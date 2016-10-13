pushd $BUILD_DIR

if [ ! -f binutils-$binutils_version.tar.gz ]; then
    echo -e "\n\n >>> Getting binutils into `pwd` \n"
    wget -c --trust-server-name ftp://ftp.gnu.org/gnu/binutils/binutils-$binutils_version.tar.gz
fi


if [ ! -d binutils-$binutils_version ]; then
    echo -e "\n\n >>> Extracting binutils \n"
    tar -xf binutils-$binutils_version.tar.gz
else
    echo -e "\n\n >>> SKIP: Extracting binutils  \n"
fi

if [ ! -d build_binutils ]; then
    echo -e "\n\n >>> Configuring binutils \n"
    mkdir -p build_binutils
    cd build_binutils
    ../binutils-$binutils_version/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --disable-werror

    echo -e "\n\n >>> Building binutils \n" 
    make $num_jobs

    echo -e "\n\n >>> Installing binutils \n"    
    make install

else
    echo -e "\n\n >>> SKIP: Configure / build binutils. Seems to be there  \n"
fi

popd
