#!/usr/bin/python

import sys
import os

includeos_src = os.environ['INCLUDEOS_SRC']
sys.path.insert(0,includeos_src + "/test")

from subprocess import call

import vmrunner

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

def test_serial_port():
  print "<test.py> Test triggered"
  global vm
  vm.writeline("Here is a test\n")


vm.on_output("trigger_test_serial_port", test_serial_port)

# Boot the VM
vm.make().boot(80)
