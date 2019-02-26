#! /usr/bin/env python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
vm = vmrunner.vms[0]

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(10,image_name='posix_utsname').clean()
