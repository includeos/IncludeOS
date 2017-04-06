#! /bin/bash

[ -z ${INCLUDEOS_PREFIX} ] && export INCLUDEOS_PREFIX="/usr/local";

# Where to link stuff
INCLUDEOS_BIN=$INCLUDEOS_PREFIX/includeos/bin

echo -e "\n###########################################"
echo -e " IncludeOS dependency installation for Mac"
echo -e "###########################################"

echo -e "\n# Prequisites:\n
    - homebrew (Mac package manager - https://brew.sh)
    - \`/usr/local\` directory with write access
    - \`/usr/local/bin\` added to your PATH
    - (Recommended) Xcode CLT (Command Line Tools)"

### DEPENDENCIES ###

# Make sure brew is installed
command -v brew >/dev/null 2>&1 || { echo >&2 " Cannot find brew! Visit http://brew.sh/ for how-to install. Aborting."; exit 1; }
# Try to update brew
echo -e "\n>>> Making sure homebrew is up to date ..."
brew update

echo -e "\n# Dependencies:\n"
## llvm (clang 3.8) ##
BREW_LLVM=homebrew/versions/llvm38
echo -e " llvm38\t\t - clang 3.8 (compiler)"

## nasm ##
BREW_NASM=nasm
echo -e " $BREW_NASM\t\t - assembler"

## cmake ##
BREW_CMAKE=cmake
echo -e " $BREW_CMAKE\t\t - for building OS and services"

## jq ##
BREW_JQ=jq
echo -e " $BREW_JQ"

## qemu ##
BREW_QEMU=qemu
echo -e " $BREW_QEMU\t\t - hypervisor for running unikernels"

## tuntap ##
BREW_TUNTAP=Caskroom/cask/tuntap
echo -e " tuntap\t\t - tap device for macOS (used by qemu)"

## BINUTILS ##
echo -e "\n binutils\t - xcompile tools required for building IncludeOS"
DEPENDENCY_BINUTILS=false

BINUTILS_BIN="/usr/local/bin/i686-elf-"
LD_INC=$BINUTILS_BIN"ld"
AR_INC=$BINUTILS_BIN"ar"
OBJCOPY_INC=$BINUTILS_BIN"objcopy"
STRIP_INC=$BINUTILS_BIN"strip"
RANLIB_INC=$BINUTILS_BIN"ranlib"

[[ -e  $LD_INC && -e $AR_INC && -e $OBJCOPY_INC && -e $STRIP_INC && -e $RANLIB_INC ]] && DEPENDENCY_BINUTILS=true
if ($DEPENDENCY_BINUTILS); then echo -e " > Found"; else echo -e " > Not Found"; fi

# Assume script is called from root, else it won't work..
function install_binutils {
  source ./etc/install_binutils.sh
}

## python packages (pip) ##
PIP_INSTALLED=false
PIP_MODS=(jsonschema psutil)
echo -e "\n python pip\t - for installing necessary python modules used when booting services: ${PIP_MODS[*]}"

## Check if pip is installed ##
if command -v pip >/dev/null 2>&1; then
  PIP_INSTALLED=true
  echo -e " > Found"
else
  echo -e " > Not found"
fi

## WARN ABOUT XCODE CLT ##

echo -e "\n NOTE: Cannot tell if Xcode Command Line Tools is installed - installation MAY fail if not installed."
echo -e " > Install with: xcode-select --install"

### INSTALL ###
echo
read -p "Install dependencies? [y/n]" -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]
then
    echo -e "\n\n>>> Installing dependencies ..."

    brew install $BREW_LLVM $BREW_NASM $BREW_CMAKE $BREW_JQ $BREW_QEMU $BREW_TUNTAP

    if (! $DEPENDENCY_BINUTILS); then
        install_binutils
    fi

    if (! $PIP_INSTALLED); then
      echo -e "\n>> Installing pip (with sudo)"
      curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
      sudo python get-pip.py
      rm get-pip.py
    fi

    pip install ${PIP_MODS[*]}

    echo -e "\n>>> Done installing dependencies."
fi

echo
read -p "Symlink crosscompile dependencies? [y/n]" -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]
then
  echo -e "\n\n>>> Symlinking dependencies ..."
  mkdir -p $INCLUDEOS_BIN

  SRC_CC="/usr/local/bin/clang-3.8"
  ln -sf $SRC_CC $INCLUDEOS_BIN/gcc
  echo -e ">> $SRC_CC > $INCLUDEOS_BIN/gcc"

  SRC_CXX="/usr/local/bin/clang++-3.8"
  ln -sf $SRC_CXX $INCLUDEOS_BIN/g++
  echo -e ">> $SRC_CXX > $INCLUDEOS_BIN/g++"

  SRC_BINUTILS="/usr/local/bin"
  ln -sf $SRC_BINUTILS/i686-elf-* $INCLUDEOS_BIN/
  echo -e ">> $SRC_BINUTILS/i686-elf-* > $INCLUDEOS_BIN/"

  SRC_NASM="/usr/local/bin/nasm"
  ln -sf $SRC_NASM $INCLUDEOS_BIN/nasm
  echo -e ">> $SRC_NASM > $INCLUDEOS_BIN/nasm"

  echo -e "\n>>> Done symlinking dependencies to $INCLUDEOS_BIN"
fi

echo -e "\n###########################################"
echo -e " Mac dependency installation done."
echo -e "###########################################\n"

