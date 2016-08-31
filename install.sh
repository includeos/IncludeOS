#!/bin/sh

# OPTIONS:
#
# Location of the IncludeOS repo (assumes current folder if not defined), e.g.:
# $ export INCLUDEOS_SRC=your/github/cloned/IncludeOS
export INCLUDEOS_SRC=${INCLUDEOS_SRC-`pwd`}

SYSTEM=`uname -s`

RELEASE=$([ $SYSTEM = "Darwin" ] && echo `sw_vers -productVersion` || echo `lsb_release -is`)

check_os_support() {
    SYSTEM=$1
    RELEASE=$2

    case $SYSTEM in
        "Darwin")
            return 0;
            ;;
        "Linux")
            case $RELEASE in
                "Ubuntu")
                    return 0;
                    ;;
                "Fedora")
                    export INCLUDEOS_SRC=`pwd`
                    return 0;
                    ;;
            esac
    esac
    return 1;
}

# check if system is supported at all
if ! check_os_support $SYSTEM $RELEASE; then
    echo -e ">>> Sorry <<< \n\
Currently only Ubuntu, Fedora and OSX are actively supported for *building* IncludeOS. \n\
On other Linux distros it shouldn't be that hard to get it to work - take a look at\n \
./etc/install_from_bundle.sh \n"
    exit 1
fi

# now install build requirements (compiler, etc). This was moved into
# a function of its own as it can easen the setup.
./etc/install_build_requirements.sh $SYSTEM $RELEASE

# if the --all-source parameter was given, build it the hard way
if [ "$1" = "--all-source" ]; then
    echo ">>> Installing everything from source"
    ./etc/install_all_source.sh
elif [ "Darwin" = "$SYSTEM" ]; then
        # TODO: move build dependencies to the install build requirements step
        ./etc/install_osx.sh
elif [ "Linux" = "$SYSTEM" ]; then

    echo -e "\n\n>>> Calling install_from_bundle.sh script"
    ./etc/install_from_bundle.sh

    echo -e "\n\n>>> Creating a virtual network, i.e. a bridge. (Requires sudo)"
    sudo ./etc/create_bridge.sh

    echo -e "\n\n>>> Done! Test your installation with ./test.sh"
fi

exit 0
