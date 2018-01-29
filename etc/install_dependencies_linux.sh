#!/bin/bash
#
# Install dependencies

############################################################
# OPTIONS:
############################################################

BUILD_DEPENDENCIES="curl make cmake nasm bridge-utils qemu jq python-pip g++-multilib gcc"
[ ! -z "$CC" ] && { CLANG_VERSION=${CC: -3}; } || CLANG_VERSION="5.0"
TEST_DEPENDENCIES="g++"
PYTHON_DEPENDENCIES="jsonschema psutil junit-xml filemagic"
INSTALLED_PIP=0
INSTALLED_CLANG=0

# NaCl
PIP_MODS_NACL="pystache antlr4-python2-runtime"
PYTHON_DEPENDENCIES="$PYTHON_DEPENDENCIES $PIP_MODS_NACL"

############################################################
# COMMAND LINE PROPERTIES:
############################################################

# Initialize variables:
SYSTEM=0
RELEASE=0
CHECK_ONLY=0
PRINT_INSTALL_STATUS=0
DEPENDENCIES_TO_INSTALL=build

while getopts "h?s:r:cpd:" opt; do
    case "$opt" in
    h|\?)
        printf "%s\n" "Options:"\
                "-s System: What system to install on"\
                "-r Release: What release of said system"\
				"-c Only check: Will only check what packages are needed (will always print status as well)"\
                "-p Print install status: Flag for wheter or not to print dependency status"\
                "-d Dependencies to install: [build | test | all] are the options"
        exit 0 ;;
	s)  SYSTEM=$OPTARG ;;
	r)  RELEASE=$OPTARG ;;
    c)  CHECK_ONLY=1 ; PRINT_INSTALL_STATUS=1;;
	p)  PRINT_INSTALL_STATUS=1 ;;
	d)  DEPENDENCIES_TO_INSTALL=$OPTARG ;;
    esac
done

# Figure out which dependencies to check
case "$DEPENDENCIES_TO_INSTALL" in
	build) ALL_DEPENDENCIES=$BUILD_DEPENDENCIES;;
	test) ALL_DEPENDENCIES=$TEST_DEPENDENCIES ;;
	all) ALL_DEPENDENCIES="$BUILD_DEPENDENCIES $TEST_DEPENDENCIES" ;;
esac

############################################################
# CHECK INSTALLED PACKAGES:
############################################################

if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
	printf "%-15s %-20s %s \n"\
		   "Status" "Package" "Version"\
		   "------" "-------" "-------"
	for package in $ALL_DEPENDENCIES; do
		dpkg-query -W $package > /dev/null 2>&1
		if [ $? -eq 0 ]; then
			printf '    \e[32m%-15s\e[0m %-20s %s \n'\
				"INSTALLED" $(dpkg-query -W $package)
		else
			printf '\e[31m%-15s\e[0m %-20s %s \n'\
				"MISSING" $package
			DEPENDENCIES="$DEPENDENCIES $package"
		fi
	done

	# Check clang version
	if [[ $(command -v clang-$CLANG_VERSION) ]]; then
		INSTALLED_CLANG=1
		if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
			printf "\n%s\n" "clang-$CLANG_VERSION -> INSTALLED"
	   	fi
	else
		if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
			printf "\n%s\n" "clang-$CLANG_VERSION -> MISSING"
		fi

	fi

	# Check if pip is installed
	if pip --version > /dev/null 2>&1; then
		INSTALLED_PIP=1
		if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
			printf "\n%s\n" "python pip -> INSTALLED"
		fi
		for package in $PYTHON_DEPENDENCIES; do
			pip show $package > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
					printf '     \e[32m%-15s\e[0m %-20s %s \n'\
						"INSTALLED" $(pip list 2> /dev/null | grep $package)
				fi
			else
				if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
					printf '     \e[31m%-15s\e[0m %-20s %s \n'\
						"MISSING" $package
					PYTHON_DEPS_TO_INSTALL="$PYTHON_DEPS_TO_INSTALL $package"
				fi
			fi
		done
	else
		INSTALLED_PIP=0
		DEPENDENCIES="$DEPENDENCIES python-pip"
		if [ $PRINT_INSTALL_STATUS -eq 1 ]; then
			printf "%s\n\n" "python pip -> MISSING"
		fi
	fi

	# Exits if CHECK_ONLY is set, exit code 1 if there are packages to install
	if [ $CHECK_ONLY -eq 1 ]; then
		if [[ -z "$DEPENDENCIES" && -z "$PYTHON_DEPS_TO_INSTALL" && $INSTALLED_CLANG -eq 1 ]]; then
			exit 0
		else
			exit 1
		fi
	fi
else
	DEPENDENCIES=$ALL_DEPENDENCIES
fi

############################################################
# INSTALL MISSING PACKAGES:
############################################################

echo ">>> Installing missing dependencies (requires sudo):"

case $SYSTEM in
    "Darwin")
        exit 0;
        ;;
    "Linux")
        case $RELEASE in
            "debian"|"ubuntu"|"linuxmint"|"parrot")
                DEPENDENCIES="$DEPENDENCIES"
                sudo apt-get -qq update || exit 1
                sudo apt-get -qqy install $DEPENDENCIES > /dev/null || exit 1
                ;;
            "fedora")
                # Removes g++-multilib from dependencies
                DEPENDENCIES=${DEPENDENCIES%g++-multilib}
                sudo dnf install $DEPENDENCIES || exit 1
                ;;
            "arch")
                DEPENDENCIES="$DEPENDENCIES python2 python2-jsonschema python2-psutil"
                sudo pacman -Syyu
                sudo pacman -S --needed $DEPENDENCIES
                ;;
        esac
esac

$INCLUDEOS_SRC/etc/install_clang_version.sh $CLANG_VERSION

sudo -H pip -q install $PYTHON_DEPENDENCIES
