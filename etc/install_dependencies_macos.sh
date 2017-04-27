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

INSTALLED_BREW=0
INSTALLED_BREW_PACKAGES=0 
INSTALLED_PIP=0
INSTALLED_PIP_PACKAGES=0
INSTALLED_BINUTILS=0
ALL_DEPENDENCIES="llvm38 nasm cmake jq qemu Caskroom/cask/tuntap"
PIP_MODS="jsonschema psutil"

############################################################
# COMMAND LINE PROPERTIES:
############################################################

# Initialize variables:
CHECK_ONLY=0
PRINT_INSTALL_STATUS=0

while getopts "h?cp" opt; do
    case "$opt" in
    h|\?)
        printf "%s\n" "Options:"\
				"-c Only check: Will only check what packages are needed (will always print status as well)"\
                "-p Print install status: Flag for wheter or not to print dependency status"
        exit 0 ;;
    c)  CHECK_ONLY=1 ; PRINT_INSTALL_STATUS=1;;
	p)  PRINT_INSTALL_STATUS=1 ;;
    esac
done

############################################################
# BREW DEPENDENCIES:
############################################################

# Brew itself
if brew help > /dev/null 2>&1; then
	INSTALLED_BREW=1
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "Brew -> INSTALLED"
	fi
else
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "Brew -> MISSING Visit http://brew.sh/ for how-to install. Aborting"
	fi
fi

# Individual packages installed with brew
if [ $INSTALLED_BREW -eq 1 ]; then
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "     Brew installed packages"
		printf "     %-15s %-20s %s \n"\
			   "Status" "Package" "Version"\
			   "------" "-------" "-------"
		for package in $ALL_DEPENDENCIES; do
			brew ls $package > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				printf '     \e[32m%-15s\e[0m %-20s %s \n'\
					"INSTALLED" $(brew ls --versions $package)
			elif brew cask ls $package; then
				printf '     \e[32m%-15s\e[0m %-20s %s \n'\
					"INSTALLED" $(brew cask ls --versions $package)
			else
				printf '     \e[31m%-15s\e[0m %-20s %s \n'\
					"MISSING" $package
				BREW_DEPENDENCIES_TO_INSTALL="$BREW_DEPENDENCIES_TO_INSTALL $package"
			fi
		done
	fi
	if [[ $CHECK_ONLY -eq 0 && ${#BREW_DEPENDENCIES_TO_INSTALL} -gt 1 ]]; then
		echo ">>> Installing: $BREW_DEPENDENCIES_TO_INSTALL"
		for formula in $BREW_DEPENDENCIES_TO_INSTALL; do
			brew install $formula
		done
	fi
fi

	
############################################################
# PYTHON DEPENDENCIES:
############################################################

# pip itself
if pip list >/dev/null 2>&1; then
	INSTALLED_PIP=1
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "python pip -> INSTALLED"
	fi
else
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "python pip -> MISSING"
	fi
	if [ $CHECK_ONLY -eq 0 ]; then
		curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
		sudo python get-pip.py
		rm get-pip.py
		INSTALLED_PIP=1
	fi
fi

# Individual packages installed with pip
if [ $INSTALLED_PIP -eq 1 ]; then
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "     pip installed packages"
		printf "     %-15s %-20s %s \n"\
			   "Status" "Package" "Version"\
			   "------" "-------" "-------"
		for package in $PIP_MODS; do
			python -c "import $package"> /dev/null 2>&1
			if [ $? -eq 0 ]; then
				printf '     \e[32m%-15s\e[0m %-20s %s \n'\
					"INSTALLED" $(pip list 2> /dev/null | grep $package)
			else
				printf '     \e[31m%-15s\e[0m %-20s %s \n'\
					"MISSING" $package
				PIP_MODS_TO_INSTALL="$PIP_MODS_TO_INSTALL $package"
			fi
		done
	fi
	if [[ $CHECK_ONLY -eq 0 && ${#PIP_MODS_TO_INSTALL} -gt 1 ]]; then
		echo ">>> Installing: $PIP_MODS"
		sudo pip install ${PIP_MODS[*]}
	fi
fi


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
	INSTALLED_BINUTILS=1
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "binutils -> INSTALLED"
	fi
else
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "binutils -> MISSING"
	fi
	if [ $CHECK_ONLY -eq 0 ]; then
		# Assume script is called from root, else it won't work..
		$INCLUDEOS_SRC/etc/install_binutils.sh
		INSTALLED_BINUTILS=1
	fi
fi

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


############################################################
# EXIT:
############################################################



echo -e Brew installed: $INSTALLED_BREW
echo -e Brew dependencies installed: $INSTALLED_BREW
echo -e Pip installed: $INSTALLED_PIP
echo -e Pip installed: $INSTALLED_PIP
echo -e Binutils installed: $INSTALLED_BINUTILS
