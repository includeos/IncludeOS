#!/usr/bin/python

import sys
sys.path.insert(0,"..")

from subprocess import call

import vmrunner

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

def cleanup():
  vm.make(["clean"])
  call(["./fat16_disk.sh", "clean"])

# Setup disk
call(["./fat16_disk.sh"], shell=True)

# Clean up on exit
vm.on_exit(cleanup)

# Boot the VM
vm.make().boot()
