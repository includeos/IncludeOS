#!/usr/bin/env python3
from __future__ import print_function
from builtins import str
import sys
import subprocess
import os

thread_timeout = 20

from vmrunner import vmrunner

disks = ["memdisk", "virtio1", "virtio2"]

# Create all data disk images from folder names
for disk in disks:
  subprocess.check_call(["./create_disk.sh", disk, disk])
vm = vmrunner.vms[0]

if len(sys.argv) > 1:
    vm.boot(thread_timeout,image_name=str(sys.argv[1]))
else:
    vm.boot(thread_timeout,image_name='fs_vfs.elf.bin')
