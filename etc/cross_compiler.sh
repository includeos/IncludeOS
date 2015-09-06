# Bash utils
. ./etc/bash_functions.sh

echo -e "\n\n >>> Creating build-directory "$build_dir "\n"
mkdir -p $build_dir
cd $build_dir

echo -e "\n\n >>> Getting binutils into `pwd` \n"
wget -c --trust-server-name ftp://ftp.uninett.no/pub/gnu/binutils/binutils-$binutils_version.tar.gz

echo -e "\n\n >>> Getting GCC \n"
wget -c --trust-server-name ftp://ftp.uninett.no/pub/gnu/gcc/gcc-$gcc_version/gcc-$gcc_version.tar.gz

if [ ! -d binutils-$binutils_version ]; then
    echo -e "\n\n >>> Extracting binutils \n"
    tar -xf binutils-$binutils_version.tar.gz
else
    echo -e "\n\n >>> SKIP: Extracting binutils  \n"
fi

if [ ! -d build-binutils ]; then
    echo -e "\n\n >>> Configuring binutils \n"
    mkdir -p build-binutils
    cd build-binutils
    ../binutils-$binutils_version/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --disable-werror
    
    echo -e "\n\n >>> Building binutils \n" 
    make $num_jobs
    or_die "Couldn't build binutils"
    
    echo -e "\n\n >>> Installing binutils \n"    
    sudo -E  make install
    or_die "Couldn't install binutils"
else
    echo -e "\n\n >>> SKIP: Configure / build binutils. Seems to be there  \n"
fi

# UNPACK GCC

if [ ! -d gcc-$gcc_version ]; then
    echo -e "\n\n >>> Unpacking GCC source \n"
    cd $build_dir
    tar -xf gcc-$gcc_version.tar.gz

    # GET GCC PREREQS 
    echo -e "\n\n >>> Getting GCC Prerequisites \n"
    cd gcc-$gcc_version/
    ./contrib/download_prerequisites
else
    echo -e "\n\n >>> SKIP: Unpacking GCC + getting prerequisites Seems to be there \n"
fi

cd $build_dir

if [ ! -d build-gcc ]; then
    mkdir -p build-gcc
    cd build-gcc

    echo -e "\n\n >>> Configuring GCC \n"
    ../gcc-$gcc_version/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
    or_die "Couldn't configure GCC"

    echo -e "\n\n >>> Building GCC \n"
    make all-gcc $num_jobs
    or_die "Couldn't build GCC"

    echo -e "\n\n >>> Installing GCC (Might require sudo) \n"
    sudo -E make install-gcc
    or_die "Couldn't install GCC"

    echo -e "\n\n >>> Building libgcc for target $TARGET \n"
    make all-target-libgcc $num_jobs
    or_die "Couldn't build libgcc"

    echo -e "\n\n >>> Installing libgcc (Might require sudo) \n"
    sudo -E make install-target-libgcc
    or_die "Couldn't install libgcc"
else
    echo -e "\n\n >>> SKIP: Building / Installing GCC + libgcc. Seems to be ok \n" 
fi
