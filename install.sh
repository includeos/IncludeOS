#!/bin/bash

SYSTEM=`uname -s`

if [ $SYSTEM == "Darwin" ]
then
    . ./etc/install_osx.sh
    exit 0
fi


FAIL_MSG=">>> Sorry <<< \n\
Currently only Ubuntu and OSX are actively supported for *building* IncludeOS. \n\
On other Linux distros it shouldn't be that hard to get it to work - take a look at\n \
./etc/install_from_bundle.sh \n"

if [ $SYSTEM != "Linux" ]
then
    echo -e $FAIL_MSG
    exit 1
fi

RELEASE=`lsb_release -is`
if [ $RELEASE != "Ubuntu" -a $RELEASE != "Fedora" ]
then
    echo -e $FAIL_MSG
    exit 1
fi

if [ "$1" == "--all-source" ]
then
    echo ">>> Installing everything from source"
    . ./etc/install_all_source.sh
else
    if [ $RELEASE == "Ubuntu" ]
    then
        echo ">>> Installing from bundle"
        . ./etc/install_ubuntu.sh
    else
        echo ">>> Installing from bundle"
        . ./etc/install_fedora.sh
    fi
fi
