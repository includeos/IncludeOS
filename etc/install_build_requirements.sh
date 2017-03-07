#!/bin/sh
#
# Install dependencies

SYSTEM=$1
RELEASE=$2

DEPENDENCIES="curl make clang cmake nasm bridge-utils qemu jq python-jsonschema python-psutil"

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
