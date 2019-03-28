#! /usr/bin/env python

from builtins import str
import sys
import os
import subprocess

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
