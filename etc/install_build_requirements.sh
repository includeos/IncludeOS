#!/bin/sh
#
# Install dependencies

# TODO: add cmake as dependency

SYSTEM=$1
RELEASE=$2

case $SYSTEM in
    "Darwin")
        exit 0;
        ;;
    "Linux")
        case $RELEASE in
            "Debian"|"Ubuntu"|"LinuxMint")
                VERSION=`lsb_release -rs`
                if [ $(awk 'BEGIN{ print "'$VERSION'"<"'16.04'" }') -eq 1 ]; then
                    if [ $RELEASE == "Ubuntu" ] ; then
                        clang_version="3.6"
                        DEPENDENCIES="gcc-5 g++-5"
                        sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test || exit 1
                    else
                        echo "deb http://ftp.debian.org/debian jessie-backports main" > /etc/apt/sources.list.d/includeOS-requirements.list
                        clang_version="3.8"
                    fi
                else
                    clang_version="3.8"
                fi

                DEPENDENCIES="curl make clang-$clang_version nasm bridge-utils qemu jq cmake $DEPENDENCIES"
                echo ">>> Installing dependencies (requires sudo):"
                echo "    Packages: $DEPENDENCIES"
                sudo apt-get update || exit 1
                sudo apt-get install -y $DEPENDENCIES || exit 1
                exit 0;
                ;;
            "Fedora")
                DEPENDENCIES="curl make clang nasm bridge-utils qemu jq python-jsonschema cmake"
                echo ">>> Installing dependencies (requires sudo):"
                echo "    Packages: $DEPENDENCIES"
                sudo dnf install $DEPENDENCIES || exit 1
                exit 0;
                ;;
            "Arch")
                DEPENDENCIES="curl make clang nasm bridge-utils qemu jq cmake"
                echo ">>> Installing dependencies (requires sudo):"
                echo "    Packages: $DEPENDENCIES"
                sudo pacman -Syyu
                sudo pacman -S $DEPENDENCIES
                exit 0;
                ;;
        esac
esac

exit 1
