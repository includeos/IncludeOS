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

TARGET_TRIPLE=$ARCH-pc-linux-elf
INSTALLED_BREW=0
INSTALLED_BREW_PACKAGES=0
INSTALLED_PIP=0
INSTALLED_PIP_PACKAGES=0
INSTALLED_BINUTILS=0
INSTALLED_SYMLINKING=0
CLANG_VERSION=5
BREW_LLVM="llvm@$CLANG_VERSION"
ALL_DEPENDENCIES="$BREW_LLVM nasm cmake jq qemu Caskroom/cask/tuntap libmagic"
PIP_MODS="jsonschema psutil filemagic"

# NaCl
PIP_MODS_NACL="pystache antlr4-python2-runtime"
PIP_MODS="$PIP_MODS $PIP_MODS_NACL"

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
		exit 1
	fi
fi

# Individual packages installed with brew
if [ $INSTALLED_BREW -eq 1 ]; then
	installed_packages=0
	not_installed_packages=0
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "     Brew installed packages"
		printf "     %-15s %-20s %s \n"\
			   "Status" "Package" "Version"\
			   "------" "-------" "-------"
	fi
	for package in $ALL_DEPENDENCIES; do
		brew ls $package > /dev/null 2>&1
		if [ $? -eq 0 ]; then
			installed_packages=$((++installed_packages))
			if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
				printf '     \e[32m%-15s\e[0m %-20s %s \n'\
					"INSTALLED" $(brew ls --versions $package)
			fi
		elif brew cask ls $package > /dev/null 2>&1; then
			installed_packages=$((++installed_packages))
			if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
				printf '     \e[32m%-15s\e[0m %-20s %s \n'\
					"INSTALLED" $(brew cask ls --versions $package)
			fi
		else
			not_installed_packages=$((++not_installed_packages))
			BREW_DEPENDENCIES_TO_INSTALL="$BREW_DEPENDENCIES_TO_INSTALL $package"
			if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
				printf '     \e[31m%-15s\e[0m %-20s %s \n'\
					"MISSING" $package
			fi
		fi
	done
	# If not all brew packages are installed they have to be installed
	if [[ $not_installed_packages -gt 0 ]]; then
		INSTALLED_BREW_PACKAGES=0
		if [[ $CHECK_ONLY -eq 0 ]]; then
			echo ">>> Installing: $BREW_DEPENDENCIES_TO_INSTALL"
			for formula in $BREW_DEPENDENCIES_TO_INSTALL; do
				brew install $formula
			done
			INSTALLED_BREW_PACKAGES=1
		fi
	else
		INSTALLED_BREW_PACKAGES=1
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
	installed_packages=0
	not_installed_packages=0
	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		printf "%s\n" "     pip installed packages"
		printf "     %-15s %-20s %s \n"\
			   "Status" "Package" "Version"\
			   "------" "-------" "-------"
	fi
	for package in $PIP_MODS; do
		pip show $package > /dev/null 2>&1
		if [ $? -eq 0 ]; then
			installed_packages=$((++installed_packages))
			if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
				printf '     \e[32m%-15s\e[0m %-20s %s \n'\
					"INSTALLED" $(pip list 2> /dev/null | grep $package)
			fi
		else
			not_installed_packages=$((++not_installed_packages))
			PIP_MODS_TO_INSTALL="$PIP_MODS_TO_INSTALL $package"
			echo b: $PIP_MODS_TO_INSTALL
			if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
				printf '     \e[31m%-15s\e[0m %-20s %s \n'\
					"MISSING" $package
			fi
		fi
	done
	if [[ $not_installed_packages -gt 0 ]]; then
		INSTALLED_PIP_PACKAGES=0
		if [[ $CHECK_ONLY -eq 0 ]]; then
			echo ">>> Installing: $PIP_MODS_TO_INSTALL"
			pip install --user ${PIP_MODS_TO_INSTALL[*]}
			INSTALLED_PIP_PACKAGES=1
		fi
	else
		INSTALLED_PIP_PACKAGES=1
	fi
fi


############################################################
# BINUTILS:
############################################################

# Check if binutils is installed, if not it will be built and installed
BINUTILS_BIN="$INCLUDEOS_PREFIX/includeos/bin/$TARGET_TRIPLE-"
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

if [[ $INSTALLED_BREW_PACKAGES -eq 1 ]]; then
  mkdir -p $INCLUDEOS_BIN

  CLANG_PATH=$(brew --prefix $BREW_LLVM)

	SRC_CC=$CLANG_PATH/bin/clang
	ln -sf $SRC_CC $INCLUDEOS_BIN/gcc

	SRC_CXX=$CLANG_PATH/bin/clang++
	ln -sf $SRC_CXX $INCLUDEOS_BIN/g++

	SRC_NASM=$(which nasm)
	ln -sf $SRC_NASM $INCLUDEOS_BIN/nasm
	INSTALLED_SYMLINKING=1

	if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
		echo -e "\n>>> Symlinking dependencies ..."
		echo -e ">> $SRC_CC > $INCLUDEOS_BIN/gcc"
		echo -e ">> $SRC_CXX > $INCLUDEOS_BIN/g++"
		echo -e ">> $SRC_NASM > $INCLUDEOS_BIN/nasm"
	fi
fi

############################################################
# EXIT:
############################################################

total_errors=$(( 5 - INSTALLED_BREW - INSTALLED_BREW_PACKAGES - INSTALLED_PIP - INSTALLED_PIP_PACKAGES - INSTALLED_BINUTILS ))
exit $total_errors
