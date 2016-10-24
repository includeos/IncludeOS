#!/bin/bash

INCLUDEOS_INSTALL=${INCLUDEOS_INSTALL-$HOME/IncludeOS_install}
mkdir -p $INCLUDEOS_INSTALL/etc
cp $PWD/to-be-installed/* $INCLUDEOS_INSTALL/etc/
