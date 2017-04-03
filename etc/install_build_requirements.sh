#!/bin/bash
#
# Install dependencies

############################################################
# OPTIONS:
############################################################

BUILD_DEPENDENCIES="curl make clang cmake nasm bridge-utils qemu jq python-jsonschema python-psutil"
TEST_DEPENDENCIES="g++ g++-multilib python-junit.xml"

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
	build) ALL_DEPENDENCIES=$BUILD_DEPENDENCIES ;;
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
			printf '\e[32m%-15s\e[0m %-20s %s \n'\
				"INSTALLED" $(dpkg-query -W $package)
		else
			printf '\e[31m%-15s\e[0m %-20s %s \n'\
				"MISSING" $package
			DEPENDENCIES="$DEPENDENCIES $package"
		fi
	done
	# Exits if CHECK_ONLY is set, exit code 1 if there are packages to install
	if [ $CHECK_ONLY -eq 1 ]; then
		if [ -z "$DEPENDENCIES" ]; then
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

case $SYSTEM in
    "Darwin")
        exit 0;
        ;;
    "Linux")
		echo ">>> Installing missing dependencies (requires sudo):"
        case $RELEASE in
            "debian"|"ubuntu"|"linuxmint")
                DEPENDENCIES="$DEPENDENCIES"
                sudo apt-get -qq update || exit 1
                sudo apt-get -qqy install $DEPENDENCIES > /dev/null || exit 1
                exit 0;
                ;;
            "fedora")
                DEPENDENCIES="$DEPENDENCIES"
                sudo dnf install $DEPENDENCIES || exit 1
                exit 0;
                ;;
            "arch")
                DEPENDENCIES="$DEPENDENCIES python2 python2-jsonschema python2-psutil"
                sudo pacman -Syyu
                sudo pacman -S --needed $DEPENDENCIES
                exit 0;
                ;;
        esac
esac

exit 1
