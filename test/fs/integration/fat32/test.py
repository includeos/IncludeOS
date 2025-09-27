#!/usr/bin/env python3
from __future__ import print_function
import sys
import os
import subprocess

thread_timeout = 30
from vmrunner import vmrunner

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Setup disk
subprocess.call(["./fat32_disk.sh"], shell=True, timeout=thread_timeout)

# Boot the VM
if len(sys.argv) > 1:
    vm.boot(thread_timeout,image_name='fs_fat32.elf.bin')
else:
    vm.boot(thread_timeout,image_name='fs_fat32.elf.bin')
