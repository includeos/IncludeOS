#! /usr/bin/env python

from builtins import str
import sys
import os

from vmrunner import vmrunner
vm = vmrunner.vms[0]

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(10,image_name='posix_utsname').clean()
