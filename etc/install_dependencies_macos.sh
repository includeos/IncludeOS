#! /bin/bash

############################################################
# OPTIONS:
############################################################
# Brew, llvm38, nasm, cmake, jq, qemu, tuntap, binutils, python pip, Xcode CLT
#BUILD_DEPENDENCIES="curl make clang cmake nasm bridge-utils qemu jq python-jsonschema python-psutil"
#TEST_DEPENDENCIES="g++ g++-multilib python-junit.xml"
export INCLUDEOS_SRC=${INCLUDEOS_SRC:-"~/IncludeOS"}
export INCLUDEOS_PREFIX=${INCLUDEOS_PREFIX:-"/usr/local"}
export INCLUDEOS_BIN=$INCLUDEOS_PREFIX/includeos/bin # Where to link stuff 

echo -e "\n###########################################"
echo -e " IncludeOS dependency installation for Mac"
echo -e "###########################################"

echo -e "\n# Prequisites:\n
    - homebrew (Mac package manager - https://brew.sh)
    - \`/usr/local\` directory with write access
    - \`/usr/local/bin\` added to your PATH
    - (Recommended) Xcode CLT (Command Line Tools)"

############################################################
# BREW DEPENDENCIES:
############################################################

BUILD_DEPENDENCIES="homebrew/versions/llvm38 nasm cmake jq qemu Caskroom/cask/tuntap"

# Brew update
if ! brew update; then
    printf "%s\n" ">>> Sorry <<<"\
		   "Cannot find brew! Visit http://brew.sh/ for how-to install. Aborting"
	exit 1
fi

echo ">>> Installing: $BUILD_DEPENDENCIES"
for formula in $BUILD_DEPENDENCIES; do
	brew install $formula
done

############################################################
# BINUTILS:
############################################################

# Check if binutils is installed, if not it will be built and installed
BINUTILS_BIN="$INCLUDEOS_PREFIX/bin/i686-elf-"
LD_INC=$BINUTILS_BIN"ld"
AR_INC=$BINUTILS_BIN"ar"
OBJCOPY_INC=$BINUTILS_BIN"objcopy"
STRIP_INC=$BINUTILS_BIN"strip"
RANLIB_INC=$BINUTILS_BIN"ranlib"

if [[ -e  $LD_INC && -e $AR_INC && -e $OBJCOPY_INC && -e $STRIP_INC && -e $RANLIB_INC ]]; then
	echo -e " > Binutils is present"
else
	# Assume script is called from root, else it won't work..
	$INCLUDEOS_SRC/etc/install_binutils.sh
fi


############################################################
# PYTHON DEPENDENCIES:
############################################################

PIP_MODS="jsonschema psutil"
echo -e "\n python pip\t - for installing necessary python modules used when booting services: ${PIP_MODS[*]}"

## Check if pip is installed ##
if command -v pip >/dev/null 2>&1; then
	echo -e " > Found"
	pip install ${PIP_MODS[*]}
else
	echo -e " > Not found"
	echo -e "\n>> Installing pip (with sudo)"
	curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
	sudo python get-pip.py
	rm get-pip.py
fi

############################################################
# XCODE CLT:
############################################################
## WARN ABOUT XCODE CLT ##

echo -e "\n NOTE: Cannot tell if Xcode Command Line Tools is installed - installation MAY fail if not installed."
echo -e " > Install with: xcode-select --install"


############################################################
# SYMLINK CROSSCOMPILE DEPENDENCIES:
############################################################

echo -e "\n\n>>> Symlinking dependencies ..."
mkdir -p $INCLUDEOS_BIN

SRC_CC=$(which clang-3.8)
ln -sf $SRC_CC $INCLUDEOS_BIN/gcc
echo -e ">> $SRC_CC > $INCLUDEOS_BIN/gcc"

SRC_CXX=$(which clang++-3.8)
ln -sf $SRC_CXX $INCLUDEOS_BIN/g++
echo -e ">> $SRC_CXX > $INCLUDEOS_BIN/g++"

SRC_BINUTILS="$INCLUDEOS_PREFIX/bin"
ln -sf $SRC_BINUTILS/i686-elf-* $INCLUDEOS_BIN/
echo -e ">> $SRC_BINUTILS/i686-elf-* > $INCLUDEOS_BIN/"

SRC_NASM=$(which nasm)
ln -sf $SRC_NASM $INCLUDEOS_BIN/nasm
echo -e ">> $SRC_NASM > $INCLUDEOS_BIN/nasm"

echo -e "\n>>> Done symlinking dependencies to $INCLUDEOS_BIN"

echo -e "\n###########################################"
echo -e " Mac dependency installation done."
echo -e "###########################################\n"
