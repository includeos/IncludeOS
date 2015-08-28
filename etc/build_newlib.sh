# Configure for an "unspecified x86 elf" target, 
# (using no prefix to denote location, we'll levave the libs here for now)

# Utils
. ./bash_functions.sh

cd $build_dir

echo -e "\n\n >>> Getting newlib \n"
wget -c --trust-server-name ftp://sourceware.org/pub/newlib/newlib-$newlib_version.tar.gz

echo -e "\n\n >>> Extracting newlib \n"
tar -xf newlib-$newlib_version.tar.gz

# MOVED UP to IncludeOS prereqs. Gcc asks for texinfo as well
# echo -e "\n\n >>> Installing dependencies"
# sudo apt-get install -y texinfo


echo -e "\n\n >>> Configuring newlib \n"
mkdir -p build_newlib
cd build_newlib

# Clean out config cache in case the cross-compiler has changed
make distclean
../newlib-$newlib_version/configure --target=$TARGET --prefix=$PREFIX

#It expects the c compiler to be called 'i686-elf-cc', but ours is called gcc.
shopt -s expand_aliases
alias i686-elf-cc="$PREFIX/bin/i686-elf-gcc"


echo ""
echo $PATH
export PATH=$PATH:$PREFIX/bin

echo -e "\n\n >>> BUILDING NEWLIB \n\n"

#i686-elf-cc --version
make all 

echo -e "\n\n >>> INSTALLING NEWLIB \n\n"
#shopt -s expand_aliases
sudo -E PATH=$PATH:$PREFIX/bin make install


