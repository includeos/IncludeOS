#! /bin/bash

# Install the IncludeOS libraries (i.e. IncludeOS_home) from binary bundle
# ...as opposed to building them all from scratch, which takes a long time
#
#
# OPTIONS:
#
# Location of the IncludeOS repo:
# $ export INCLUDEOS_SRC=your/github/cloned/IncludeOS
#
# Parent directory of where you want the IncludeOS libraries (i.e. IncludeOS_home)
# $ export INCLUDEOS_INSTALL_LOC=parent/folder/for/IncludeOS/libraries i.e.

[[ -z $INCLUDEOS_SRC ]] && export INCLUDEOS_SRC=`pwd`
[[ -z $INCLUDEOS_INSTALL_LOC ]] && export INCLUDEOS_INSTALL_LOC=$HOME
export INCLUDEOS_HOME=$INCLUDEOS_INSTALL_LOC/IncludeOS_install
export INCLUDEOS_BUILD=$INCLUDEOS_INSTALL_LOC/IncludeOS_build


echo -e "\n###################################"
echo -e "IncludeOS installation for Mac OS X"
echo -e "###################################"

echo -e "\n# Prequisites:\n
    - homebrew (OS X package manager - https://brew.sh)
    - \`/usr/local\` directory with write access
    - \`/usr/local/bin\` added to your PATH
    - (Recommended) XCode CLT (Command Line Tools)"

### DEPENDENCIES ###

echo -e "\n# Dependencies"

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
echo -e "\nbinutils (ld, ar, objcopy) - required for building IncludeOS"
DEPENDENCY_BINUTILS=false

BINUTILS_DIR=$INCLUDEOS_BUILD/binutils
LINKER_PREFIX=i686-elf-
BINUTILS_LD=$BINUTILS_DIR/bin/$LINKER_PREFIX"ld"
BINUTILS_AR=$BINUTILS_DIR/bin/$LINKER_PREFIX"ar"
BINUTILS_OBJCOPY=$BINUTILS_DIR/bin/$LINKER_PREFIX"objcopy"

# Make directory inside IncludeOS_install to store ld, ar and objcopy
INCLUDEOS_BIN=$INCLUDEOS_HOME/bin
mkdir -p $INCLUDEOS_BIN

# For copying ld, ar and objcopy
LD_INC=$INCLUDEOS_BIN/"ld"
AR_INC=$INCLUDEOS_BIN/"ar"
OBJCOPY_INC=$INCLUDEOS_BIN/"objcopy"
[[ -e  $LD_INC && -e $AR_INC && -e $OBJCOPY_INC ]] && DEPENDENCY_BINUTILS=true
if ($DEPENDENCY_BINUTILS); then echo -e "> Found"; else echo -e "> Not Found"; fi

function install_binutils {
    echo -e "\n>>> Installing: binutils"

    # Create build directory if not exist
    mkdir -p $INCLUDEOS_BUILD

    # Decide filename (release)
    BINUTILS_RELEASE=binutils-2.25
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
if ! [[ $(xcode-select -p) ]]
then
    echo -e "\nWARNING: Command Line Tools don't seem to be installed, installation MAY not complete.
    Install with: xcode-select --install"
fi

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


### INSTALL BINARY RELEASE ###

echo -e "\n\n# Installing binary release"
echo ">>> Updating git-tags "
# Get the latest tag from IncludeOS repo
pushd $INCLUDEOS_SRC
git fetch --tags
tag=`git describe --abbrev=0`

echo -e "\n>>> Fetching git submodules "
git submodule init
git submodule update

popd

filename_tag=`echo $tag | tr . -`
filename="IncludeOS_install_"$filename_tag".tar.gz"

# Note: Maybe this can be extracted from install_osx.sh?
# If the tarball exists, use that
if [ -e $filename ]
then
    echo -e "\n\n>>> IncludeOS tarball exists - extracting to $INCLUDEOS_INSTALL_LOC"
else
    echo -e "\n\n>>> Downloading IncludeOS release tarball from GitHub"
    # Download from GitHub API
    if [ "$1" = "-oauthToken" ]
    then
        oauthToken=$2
        echo -e "\n\n>>> Getting the ID of the latest release from GitHub"
        JSON=`curl -u $git_user:$oauthToken https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`
    else
        echo -e "\n\n>>> Getting the ID of the latest release from GitHub"
        JSON=`curl https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`
    fi
    ASSET=`echo $JSON | $INCLUDEOS_SRC/etc/get_latest_binary_bundle_asset.py`
    ASSET_URL=https://api.github.com/repos/hioa-cs/IncludeOS/releases/assets/$ASSET

    echo -e "\n\n>>> Getting the latest release bundle from GitHub"
    if [ "$1" = "-oauthToken" ]
    then
        curl -H "Accept: application/octet-stream" -L -o $filename -u $git_user:$oauthToken $ASSET_URL
    else
        curl -H "Accept: application/octet-stream" -L -o $filename $ASSET_URL
    fi

    echo -e "\n\n>>> Fetched tarball - extracting to $INCLUDEOS_INSTALL_LOC"
fi

# Install
gzip -c $filename | tar xopf - -C $INCLUDEOS_INSTALL_LOC


### Define linker and archiver

# Binutils ld & ar
export LD_INC=$LD_INC
export AR_INC=$AR_INC
export OBJCOPY_INC=$OBJCOPY_INC

echo -e "\n\n>>> Building IncludeOS"
pushd $INCLUDEOS_SRC/src
make -j
make install
popd

echo -e "\n\n>>> Compiling the vmbuilder, which makes a bootable vm out of your service"
pushd $INCLUDEOS_SRC/vmbuild
make
cp vmbuild $INCLUDEOS_HOME/
popd

$INCLUDEOS_SRC/etc/copy_scripts.sh

echo -e "\n### OS X installation done. ###"

echo -e "\nTo build services and run tests, set LD_INC:"
echo -e "export LD_INC=$LD_INC"

echo -e "\nTo rebuild IncludeOS, set AR_INC and OBJCOPY_INC (in addtion to LD_INC):"
echo -e "export AR_INC=$AR_INC && export OBJCOPY_INC=$OBJCOPY_INC"

echo -e "\nTo run services, see: ./etc/vboxrun.sh. (VirtualBox needs to be installed)\n"
