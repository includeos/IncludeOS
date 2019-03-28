#! /usr/bin/env python
from __future__ import print_function
from builtins import str
import sys
import os
import subprocess
import subprocess32

thread_timeout = 30

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

def cleanup():
    # Call the cleanup script - let python do the printing to get it synced
    print(subprocess.check_output(["./fat32_disk.sh", "clean"]))

# Setup disk
subprocess32.call(["./fat32_disk.sh"], shell=True, timeout=thread_timeout)

# Clean up on exit
vm.on_exit(cleanup)

# Boot the VM
if len(sys.argv) > 1:
    vm.boot(thread_timeout,image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(thread_timeout,image_name='fs_ide').clean()
