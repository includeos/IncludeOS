#!/usr/bin/python

import sys
sys.path.insert(0,"..")

from subprocess import call

def cleanup():
  call(["./test.sh", "clean"])

# Setup disk
call(["./test.sh"], shell=True)

import vmrunner

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Clean up on exit
vm.on_exit(cleanup)

# Boot the VM
vm.make().boot()
