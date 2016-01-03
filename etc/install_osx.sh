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
    - homebrew (OSX package manager - https://brew.sh) 
    - \`/usr/local\` directory with write access
    - \`/usr/local/bin\` added to your PATH"

### DEPENDENCIES ###

echo -e "\n# Dependencies"

## LLVM ##
echo -e "\nllvm36 (clang/clang++ 3.6) - required for compiling"
DEPENDENCY_LLVM=false

BREW_LLVM=llvm36
BREW_CLANG_CC=/usr/local/bin/clang-3.6
BREW_CLANG_CPP=/usr/local/bin/clang++-3.6

[ -e $BREW_CLANG_CPP ] && DEPENDENCY_LLVM=true
if ($DEPENDENCY_LLVM); then echo -e "> Found"; else echo -e "> Not Found"; fi

function install_llvm {
    echo -e "\n>>> Installing: llvm"
    # Check if brew is installed
    command -v brew >/dev/null 2>&1 || { echo >&2 " Cannot find brew! Visit http://brew.sh/ for how-to install. Aborting."; exit 1; }
    # Install llvm
    echo -e "\n> Install $BREW_LLVM with brew"
    brew install $BREW_LLVM
    echo -e "\n>>> Done installing: llvm"
}


## BINUTILS ##
echo -e "\nbinutils (ld, ar) - required for linker and building archives"
DEPENDENCY_BINUTILS=false

BINUTILS_DIR=$INCLUDEOS_BUILD/binutils
LINKER_PREFIX=i686-elf-
BINUTILS_LD=$BINUTILS_DIR/bin/$LINKER_PREFIX"ld"
BINUTILS_AR=$BINUTILS_DIR/bin/$LINKER_PREFIX"ar"

# For copying ld into /usr/local/bin
LD_INCLUDEOS=ld-i686
LD_INCLUDEOS_FULL_PATH=/usr/local/bin/$LD_INCLUDEOS
LD_EXISTS=`command -v $LD_INCLUDEOS`
[[ -e  $LD_EXISTS && -e $BINUTILS_AR ]] && DEPENDENCY_BINUTILS=true
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
    make -j4 --silent
    make install
    popd

    ## Clean up
    echo -e "\n> Cleaning up..."
    rm -rf $INCLUDEOS_BUILD/$BINUTILS_RELEASE

    ## Create link
    echo -e "\n> Copying linker ($BINUTILS_LD) => $LD_INCLUDEOS_FULL_PATH"
    cp $BINUTILS_LD $LD_INCLUDEOS_FULL_PATH

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


### INSTALL ###
echo
read -p "Install missing dependencies? " -n 1 -r
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
popd 

filename_tag=`echo $tag | tr . -`
filename="IncludeOS_install_"$filename_tag".tar.gz"

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
export LD_INC=$LD_INCLUDEOS
export AR_INC=$BINUTILS_AR

echo -e "\n\n>>> Building IncludeOS"
pushd $INCLUDEOS_SRC/src
make -j4
make install
popd

echo -e "\n\n>>> Compiling the vmbuilder, which makes a bootable vm out of your service"
pushd $INCLUDEOS_SRC/vmbuild
make
cp vmbuild $INCLUDEOS_HOME/
popd

echo -e "\n\n>>> Done! Test your installation with ./test.sh"

echo -e "\n### OSX installation done. ###"
echo -e "\nTo build services and run tests, use the following before building:"
echo -e "export LD_INC=$LD_INCLUDEOS"

echo -e "\nTo run services, see: ./etc/vboxrun.sh. (VirtualBox needs to be installed)\n"

