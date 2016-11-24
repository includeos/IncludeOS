#! /bin/bash

# Install the IncludeOS libraries (i.e. IncludeOS_install) from binary bundle
# ...as opposed to building them all from scratch, which takes a long time
#
#
# OPTIONS:
#
# Location of the IncludeOS repo:
# $ export INCLUDEOS_SRC=your/github/cloned/IncludeOS
#
# Parent directory of where you want the IncludeOS libraries (i.e. IncludeOS_install)
# $ export INCLUDEOS_INSTALL_LOC=parent/folder/for/IncludeOS/libraries i.e.

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
INCLUDEOS_BUILD=${INCLUDEOS_BUILD-$HOME/IncludeOS_build}
INCLUDEOS_INSTALL_LOC=${INCLUDEOS_INSTALL_LOC-$HOME}
INCLUDEOS_INSTALL=${INCLUDEOS_INSTALL-$INCLUDEOS_INSTALL_LOC/IncludeOS_install}


echo -e "\n#########################################"
echo -e "IncludeOS dependency installation for Mac"
echo -e "#########################################"

echo -e "\n# Prequisites:\n
    - homebrew (OS X package manager - https://brew.sh)
    - \`/usr/local\` directory with write access
    - \`/usr/local/bin\` added to your PATH
    - (Recommended) Xcode CLT (Command Line Tools)"

### DEPENDENCIES ###

echo -e "\n# Dependencies"

## jq ##
brew install jq

## qemu ##
brew install qemu

## tuntap ##
brew install Caskroom/cask/tuntap

## LLVM ##
echo -e "\nllvm38 (clang/clang++ 3.8) - required for compiling"
DEPENDENCY_LLVM=false

BREW_LLVM=homebrew/versions/llvm38
BREW_CLANG_CC=/usr/local/bin/clang-3.8
BREW_CLANG_CPP=/usr/local/bin/clang++-3.8

[ -e $BREW_CLANG_CPP ] && DEPENDENCY_LLVM=true
if ($DEPENDENCY_LLVM); then echo -e "> Found"; else echo -e "> Not Found"; fi

function install_llvm {
    echo -e "\n>>> Installing: llvm"
    # Check if brew is installed
    command -v brew >/dev/null 2>&1 || { echo >&2 " Cannot find brew! Visit http://brew.sh/ for how-to install. Aborting."; exit 1; }
    # Try to update brew
    echo -e "\n> Make sure homebrew is up to date."
    brew update
    # Install llvm
    echo -e "\n> Install $BREW_LLVM with brew"
    brew install $BREW_LLVM
    echo -e "\n>>> Done installing: llvm"
}

## BINUTILS ##
echo -e "\nGNU binutils (ld, ar, objcopy, strip, ranlib) - xcompile tools required for building IncludeOS"
DEPENDENCY_BINUTILS=false


BINUTILS_BIN="/usr/local/bin/i686-elf-"
LD_INC=$BINUTILS_BIN"ld"
AR_INC=$BINUTILS_BIN"ar"
OBJCOPY_INC=$BINUTILS_BIN"objcopy"
STRIP_INC=$BINUTILS_BIN"strip"
RANLIB_INC=$BINUTILS_BIN"ranlib"

[[ -e  $LD_INC && -e $AR_INC && -e $OBJCOPY_INC && -e $STRIP_INC && -e $RANLIB_INC ]] && DEPENDENCY_BINUTILS=true
if ($DEPENDENCY_BINUTILS); then echo -e "> Found"; else echo -e "> Not Found"; fi

# Assume script is called from root, else it won't work..
MYDIR="$(dirname "$(readlink -f "$0")")" # lol
function install_binutils {
  source $MYDIR/etc/install_binutils.sh
}

## NASM ##
echo -e "\nnasm (assembler) - required for assemble bootloader"
NASM_VERSION=`nasm -v`
echo -e "> Will try to install with brew."
DEPENDENCY_NASM=false

function install_nasm {
    echo -e "\n>>> Installing: nasm"
    # Check if brew is installed
    command -v brew >/dev/null 2>&1 || { echo >&2 " Cannot find brew! Visit http://brew.sh/ for how-to install. Aborting."; exit 1; }
    # Install llvm
    echo -e "\n> Try to install nasm with brew"
    brew install nasm
    echo -e "\n>>> Done installing: nasm"
}

## WARN ABOUT XCODE CLT ##

echo -e "\nNOTE: Cannot tell if Xcode Command Line Tools is installed - installation MAY fail if not installed."
echo -e "> Install with: xcode-select --install"

### INSTALL ###
echo
read -p "Install missing dependencies? [y/n]" -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]
then
    echo -e "\n\n# Installing dependencies"

    if (! $DEPENDENCY_LLVM); then
        install_llvm
    fi

    if ( $DEPENDENCY_BINUTILS); then
        install_binutils
    fi

    install_nasm

    echo -e "\n>>> Done installing dependencies."
fi

echo -e "\n#########################################"
echo -e "Mac dependency installation done."
echo -e "#########################################"
echo -e "\nTo build IncludeOS and services with cmake, mac-toolchain.cmake needs to be used:"
echo -e "\n$ cmake -DCMAKE_TOOLCHAIN_FILE=mac-toolchain.cmake"
echo -e "\nmac-toolchain.cmake is located in <repo>/etc (when building OS),"
echo -e "and when installed, <includeos_prefix>/includeos (default /usr/local)\n"
