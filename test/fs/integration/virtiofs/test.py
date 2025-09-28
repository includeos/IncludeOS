#!/usr/bin/env python3

from __future__ import print_function
from builtins import str
import subprocess
import socket
import sys
import os

from vmrunner import vmrunner
from vmrunner.prettify import color

thread_timeout = 7

# Setup shared directory
subprocess.call(["./shared_dir.sh"], shell=True, timeout=thread_timeout)

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.boot(thread_timeout,image_name='fs_virtiofs.elf.bin')
