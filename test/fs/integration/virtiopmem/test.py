#!/usr/bin/env python3

from __future__ import print_function
import socket
import sys
import os
import subprocess

from vmrunner import vmrunner
from vmrunner.prettify import color

thread_timeout = 30

# Setup disk
subprocess.call(["./padded_image.sh"], shell=True, timeout=thread_timeout)

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.boot(120,image_name='fs_virtiopmem.elf.bin')
