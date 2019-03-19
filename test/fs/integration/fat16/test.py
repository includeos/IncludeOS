#! /usr/bin/env python

import sys
import os
from subprocess import call
from vmrunner import vmrunner

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Boot the VM
if len(sys.argv) > 1:
    vm.boot(30,image_name='fs_fat16')
else:
    vm.cmake().boot(30,image_name='fs_fat16').clean()
