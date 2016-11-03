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


echo -e "\n###################################"
echo -e "IncludeOS installation for Mac OS X"
echo -e "###################################"

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
echo -e "\nbinutils (ld, ar, objcopy, strip) - required for building IncludeOS"
DEPENDENCY_BINUTILS=false

BINUTILS_DIR=$INCLUDEOS_BUILD/binutils
BINUTILS_BIN=$BINUTILS_DIR/i686-elf/bin
BINUTILS_LD=$BINUTILS_BIN/"ld"
BINUTILS_AR=$BINUTILS_BIN/"ar"
BINUTILS_OBJCOPY=$BINUTILS_BIN/"objcopy"
BINUTILS_STRIP=$BINUTILS_BIN/"strip"

# Make directory inside IncludeOS_install to store ld, ar and objcopy
INCLUDEOS_BIN=$INCLUDEOS_INSTALL/bin
mkdir -p $INCLUDEOS_BIN

# For copying ld, ar and objcopy
LD_INC=$INCLUDEOS_BIN/"ld"
AR_INC=$INCLUDEOS_BIN/"ar"
OBJCOPY_INC=$INCLUDEOS_BIN/"objcopy"
STRIP_INC=$INCLUDEOS_BIN/"strip"
[[ -e  $LD_INC && -e $AR_INC && -e $OBJCOPY_INC && -e $STRIP_INC ]] && DEPENDENCY_BINUTILS=true
if ($DEPENDENCY_BINUTILS); then echo -e "> Found"; else echo -e "> Not Found"; fi

function install_binutils {
    echo -e "\n>>> Installing: binutils"

    # Create build directory if not exist
    mkdir -p $INCLUDEOS_BUILD

    # Decide filename (release)
    BINUTILS_RELEASE=binutils-2.27
    filename_binutils=$BINUTILS_RELEASE".tar.gz"

    # Check if file is downloaded
    if [ -e $INCLUDEOS_BUILD/$filename_binutils ]
    then
        echo -e "\n> $BINUTILS_RELEASE already downloaded."
    else
        # Download binutils
        echo -e "\n> Downloading $BINUTILS_RELEASE."
        curl https://ftp.gnu.org/gnu/binutils/$filename_binutils -o $INCLUDEOS_BUILD/$filename_binutils
    fi

    ## Unzip
    echo -e "\n> Unzip $filename_binutils to $INCLUDEOS_BUILD"
    gzip -c $INCLUDEOS_BUILD/$filename_binutils | tar xopf - -C $INCLUDEOS_BUILD

    ## Configure
    pushd $INCLUDEOS_BUILD/$BINUTILS_RELEASE

    ## Install
    echo -e "\n> Installing $BINUTILS_RELEASE to $BINUTILS_DIR"
    ./configure --program-prefix=$LINKER_PREFIX --prefix=$BINUTILS_DIR --enable-multilib --enable-ld=yes --target=i686-elf --disable-werror --enable-silent-rules
    make -j --silent
    make install
    popd

    ## Clean up
    echo -e "\n> Cleaning up..."
    rm -rf $INCLUDEOS_BUILD/$BINUTILS_RELEASE

    ## Copy LD
    echo -e "\n> Copying linker ($BINUTILS_LD) => $LD_INC"
    cp $BINUTILS_LD $LD_INC

    ## Copy AR
    echo -e "\n> Copying archiver ($BINUTILS_AR) => $AR_INC"
    cp $BINUTILS_AR $AR_INC

    ## Copy OBJCOPY
    echo -e "\n> Copying objcopy ($BINUTILS_OBJCOPY) => $OBJCOPY_INC"
    cp $BINUTILS_OBJCOPY $OBJCOPY_INC

    ## Copy STRIP
    echo -e "\n> Copying strip ($BINUTILS_STRIP) => $STRIP_INC"
    cp $BINUTILS_STRIP $STRIP_INC

    echo -e "\n>>> Done installing: binutils"
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

    if (! $DEPENDENCY_BINUTILS); then
        install_binutils
    fi

    install_nasm

    echo -e "\n>>> Done installing dependencies."
fi

### Define linker and archiver

# Binutils ld & ar
export LD_INC=$LD_INC
export AR_INC=$AR_INC
export OBJCOPY_INC=$OBJCOPY_INC
export STRIP_INC=$STRIP_INC

### INSTALL BINARY RELEASE ###
echo -e "\n\n>>> Calling install_from_bundle.sh script"
$INCLUDEOS_SRC/etc/install_from_bundle.sh

echo -e "\n### OS X installation done. ###"

echo -e "\nTo build services and run tests, set LD_INC and STRIP_INC:"
echo -e "export LD_INC=$LD_INC && export STRIP_INC=$STRIP_INC && export OBJCOPY_INC=$OBJCOPY_INC"

echo -e "\nTo rebuild IncludeOS, set AR_INC and OBJCOPY_INC (in addtion to the above):"
echo -e "export AR_INC=$AR_INC"

echo -e "\nTo run services, see: ./etc/vboxrun.sh. (VirtualBox needs to be installed)\n"
