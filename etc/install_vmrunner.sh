#!/bin/bash

# Installs the vmrunner python package

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
VMRUNNER_GLOBAL_INSTALL=${VMRUNNER_GLOBAL_INSTALL-OFF}

# Install vmrunner globally if specified
pushd $INCLUDEOS_SRC/vmrunner
if [ "$VMRUNNER_GLOBAL_INSTALL" = ON ]; then 
	sudo python setup.py install
else
	python setup.py install --user
fi
popd


