#!/bin/sh

# OPTIONS:
#
# Location of the IncludeOS repo (assumes current folder if not defined), e.g.:
# $ export INCLUDEOS_SRC=your/github/cloned/IncludeOS
export INCLUDEOS_SRC=${INCLUDEOS_SRC-`pwd`}

SYSTEM=`uname -s`

RELEASE=$([ $SYSTEM = "Darwin" ] && echo `sw_vers -productVersion` || echo `lsb_release -is`)

[ "$RELEASE" = "neon" ] && RELEASE="Ubuntu"

check_os_support() {
    SYSTEM=$1
    RELEASE=$2

    case $SYSTEM in
        "Darwin")
            return 0;
            ;;
        "Linux")
            case $RELEASE in
                "Debian"|"Ubuntu"|"LinuxMint")
                    return 0;
                    ;;
                "Fedora")
                    export INCLUDEOS_SRC=`pwd`
                    return 0;
                    ;;
                "Arch")
                    return 0;
                    ;;
            esac
    esac
    return 1;
}

# check if sudo is available
if ! command -v sudo > /dev/null 2>&1; then
    echo -e ">>> Sorry <<< \n\
The command sudo was not found. \n"
    exit 1
fi

# check if system is supported at all
if ! check_os_support $SYSTEM $RELEASE; then
    echo -e ">>> Sorry <<< \n\
Currently only Debian testing/jessie backports, Ubuntu, Fedora, Arch,\n\
and OSX are actively supported for *building* IncludeOS. \n\
On other Linux distros it shouldn't be that hard to get it to work - take\n\
a look at\n \
./etc/install_from_bundle.sh \n"
    exit 1
fi

# now install build requirements (compiler, etc). This was moved into
# a function of its own as it can easen the setup.
if ! ./etc/install_build_requirements.sh $SYSTEM $RELEASE; then
    echo -e ">>> Sorry <<< \n\
Could not install build requirements. \n"
    exit 1
fi

# if the --all-source parameter was given, build it the hard way
if [ "$1" = "--all-source" ]; then
    echo ">>> Installing everything from source"
    ./etc/install_all_source.sh

elif [ "Darwin" = "$SYSTEM" ]; then
    # TODO: move build dependencies to the install build requirements step
    ./etc/install_osx.sh

elif [ "Linux" = "$SYSTEM" ]; then
    echo -e "\n\n>>> Calling install_from_bundle.sh script"
    if ! ./etc/install_from_bundle.sh; then
        echo -e ">>> Sorry <<< \n\
Could not install from bundle. \n"
        exit 1
    fi

    echo
    if ! ./etc/create_bridge.sh; then
        echo -e ">>> Sorry <<< \n\
Could not create or configure bridge. \n"
        exit 1
    fi

    echo -e "\n\n>>> Done! Test your installation with ./test.sh"
fi

exit 0
