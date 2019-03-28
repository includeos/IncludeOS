#!/usr/bin/python

from __future__ import print_function
from builtins import str
import sys
import os

from subprocess import call

from vmrunner import vmrunner
vm = vmrunner.vms[0]

num_outputs = 0

def cleanup():
  vm.clean()

def increment(line):
  global num_outputs
  num_outputs += 1
  print("num_outputs after increment: ", num_outputs)
  return True

def check_num_outputs(line):
  assert(num_outputs == 1)
  vmrunner.vms[0].exit(0, "All tests passed")
  return True

vm.on_output("All \d+ selected tests passed", increment)

vm.on_output("All done!", check_num_outputs)

vm.on_exit(cleanup)

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vm.boot(20,image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(20,image_name='posix_file_fd').clean()
