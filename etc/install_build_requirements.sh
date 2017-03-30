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
CHECK_INSTALLED=0
DEPENDENCIES_TO_INSTALL=build

while getopts "h?s:r:cd:" opt; do
    case "$opt" in
    h|\?)
        printf "%s\n" "Options:"\
                "-s System: What system to install on"\
                "-r Release: What release of said system"\
                "-c Check installed: Flag for checking if dependencies are installed"
                "-d Dependencies to install: [build | test | all] are the options"
        exit 0 ;;
    s)  SYSTEM=$OPTARG ; shift 2 ;;
    r)  RELEASE=$OPTARG ; shift 2 ;;
    c)  CHECK_INSTALLED=1 ;;
	d)  DEPENDENCIES_TO_INSTALL=$OPTARG ; shift 2 ;;
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

if [ $CHECK_INSTALLED -eq 1 ]; then
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
fi

############################################################
# INSTALL MISSING PACKAGES:
############################################################

case $SYSTEM in
    "Darwin")
        exit 0;
        ;;
    "Linux")
		echo ">>> Installing dependencies (requires sudo):"
        case $RELEASE in
            "debian"|"ubuntu"|"linuxmint")
                DEPENDENCIES="$DEPENDENCIES"
                echo "    Packages: $DEPENDENCIES"
                sudo apt-get -qq update || exit 1
                sudo apt-get -qqy install $DEPENDENCIES > /dev/null || exit 1
                exit 0;
                ;;
            "fedora")
                DEPENDENCIES="$DEPENDENCIES"
                echo "    Packages: $DEPENDENCIES"
                sudo dnf install $DEPENDENCIES || exit 1
                exit 0;
                ;;
            "arch")
                DEPENDENCIES="$DEPENDENCIES python2 python2-jsonschema python2-psutil"
                echo "    Packages: $DEPENDENCIES"
                sudo pacman -Syyu
                sudo pacman -S --needed $DEPENDENCIES
                exit 0;
                ;;
        esac
esac

exit 1
