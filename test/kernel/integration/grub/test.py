#! /usr/bin/env python

import sys
import os
import subprocess

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner

vm = vmrunner.vms[0];

# Build, run and clean
vm.cmake()

# Cmake changes to build dir
os.chdir("..")

# Create the GRUB image
subprocess.check_call(["bash","./create_disk.sh","-c"])

# Boot the image
vm.boot(multiboot = False).clean()
