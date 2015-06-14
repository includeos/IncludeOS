# Configure for an "unspecified x86 elf" target, 
# (using no prefix to denote location, we'll levave the libs here for now)
export PREFIX="/usr/local/IncludeOS"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

LIBVER=2.2.0-1

mkdir -p $HOME/cross-dev
cd $HOME/cross-dev

echo -e "\n\n >>> Getting newlib \n"
wget -c --trust-server-name ftp://sourceware.org/pub/newlib/newlib-$LIBVER.tar.gz

echo -e "\n\n >>> Extracting "
tar -xf newlib-$LIBVER.tar.gz

echo -e "\n\n >>> Installing dependencies"
sudo apt-get install -y texinfo

echo -e "\n\n >>> Configuring \n"
mkdir -p build_newlib
cd build_newlib

# Clean out config cache in case the cross-compiler has changed
make distclean

../newlib-$LIBVER/configure --target=$TARGET --prefix=$PREFIX

#It expects the c compiler to be called 'i686-elf-cc', but ours is called gcc.
shopt -s expand_aliases
alias i686-elf-cc="$PREFIX/bin/i686-elf-gcc"

echo ""
echo $PATH
echo -e "\n\n---------- STARTING BUILD --------------\n\n"

#i686-elf-cc --version
make all install
