#!/bin/sh
#
# Install dependencies

# TODO: add cmake as dependency

SYSTEM=$1
RELEASE=$2

case $SYSTEM in
    "Darwin")
        return 0;
        ;;
    "Linux")
        case $RELEASE in
            "Ubuntu")
                UBUNTU_VERSION=`lsb_release -rs`
		if [ $(awk 'BEGIN{ print "'16.04'"<"'16.04'" }') -eq 1 ]; then
                    clang_version="3.6"
                else
                    clang_version="3.8"
                fi

                DEPENDENCIES="curl make clang-$clang_version nasm bridge-utils qemu"
                echo ">>> Installing dependencies (requires sudo):"
                echo "    Packages: $DEPENDENCIES"
                sudo apt-get update
                sudo apt-get install -y $DEPENDENCIES
                exit 0;
                ;;
            "Fedora")
                DEPENDENCIES="curl make clang nasm bridge-utils qemu"
                echo ">>> Installing dependencies (requires sudo):"
                echo "    Packages: $DEPENDENCIES"
                sudo dnf install $DEPENDENCIES
                exit 0;
                ;;
        esac
esac

exit 1
