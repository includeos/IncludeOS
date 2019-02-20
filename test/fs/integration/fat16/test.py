#! /usr/bin/env python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from subprocess import call

from vmrunner import vmrunner

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Boot the VM
if len(sys.argv) > 1:
    vm.boot(30,image_name='fs_fat16')
else:
    vm.cmake().boot(30,image_name='fs_fat16').clean()
