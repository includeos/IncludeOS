#!/bin/bash

# Installs the vmrunner python package

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
INCLUDEOS_VMRUNNER_INSTALL=${INCLUDEOS_VMRUNNER_INSTALL-GLOBAL} 

# Install vmrunner globally if specified
pushd $INCLUDEOS_SRC/vmrunner
if [ "$INCLUDEOS_VMRUNNER_INSTALL" = GLOBAL ]; then 
	sudo python setup.py install
elif [ "$INCLUDEOS_VMRUNNER_INSTALL" = DEVELOP ]; then
	python setup.py develop --user
elif [ "$INCLUDEOS_VMRUNNER_INSTALL" = LOCAL ]; then
	python setup.py install --user
else
	echo ">>>Error<<< \n Wrong INCLUDEOS_VMRUNNER_INSTALL variable, set to GLOBAL, DEVELOP OR LOCAL"
fi
popd


