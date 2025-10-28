#!/usr/bin/env python3

import sys
import os
from subprocess import call
from vmrunner import vmrunner

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Boot the VM
if len(sys.argv) > 1:
    vm.boot(30,image_name='fs_fat16.elf.bin')
else:
    vm.boot(30,image_name='fs_fat16.elf.bin')
