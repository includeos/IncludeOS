#! /usr/bin/env python
from __future__ import print_function
from builtins import str
import sys
import subprocess
import os
import subprocess32

thread_timeout = 20

from vmrunner import vmrunner

disks = ["memdisk", "virtio1", "virtio2"]

# Remove all data disk images
def cleanup():
    for disk in disks:
        diskname = disk + ".disk"
        print("Removing disk file ", diskname)
        subprocess32.check_call(["rm", "-f", diskname], timeout=thread_timeout)

# Create all data disk images from folder names
for disk in disks:
  subprocess32.check_call(["./create_disk.sh", disk, disk])
vm = vmrunner.vms[0]

vm.on_exit_success(cleanup)

if len(sys.argv) > 1:
    vm.boot(thread_timeout,image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(thread_timeout,image_name='fs_vfs').clean()
