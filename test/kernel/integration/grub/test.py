#! /usr/bin/env python

import sys
import os
import subprocess

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner

vm = vmrunner.vms[0];

if len(sys.argv) == 1:
    # Build, run and clean
    vm.cmake()

    # Cmake changes to build dir
    os.chdir("..")

# Use grubify-script
grubify = "grubiso.sh"


#TODO MOVE to cmake ?
# Boot the image
if len(sys.argv) > 1:
    # Create the GRUB image
    subprocess.check_call(["bash",grubify,str(sys.argv[1])])
else:
    # Create the GRUB image
    subprocess.check_call(["bash",grubify,"build/service"])
vm.boot(multiboot = False)
