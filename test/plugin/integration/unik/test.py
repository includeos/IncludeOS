#! /usr/bin/python

from builtins import str
import sys
import os

from vmrunner import vmrunner

# TODO: Implement a mockup of the Unik registration protocol on 10.0.0.56

vm=vmrunner.vms[0]

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.cmake().boot(60,image_name="plugin_unik").clean()
