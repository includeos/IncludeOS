# Configure for an "unspecified x86 elf" target, 
# (using no prefix to denote location, we'll levave the libs here for now)

export PREFIX="/usr/local/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

../newlib-2.1.0/configure --target=$TARGET --prefix=$PREFIX

#It expects the c compiler to be called 'i686-elf-cc', but ours is called gcc.
shopt -s expand_aliases
alias i686-elf-cc="/usr/local/cross/bin/i686-elf-gcc"

echo ""
echo $PATH
echo -e "\n\n---------- STARTING BUILD --------------\n\n"

#i686-elf-cc --version
make all install
