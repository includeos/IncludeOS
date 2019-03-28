#!/usr/bin/python

from __future__ import print_function
import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from subprocess import call

from vmrunner import vmrunner

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

def test_serial_port():
  print("<test.py> Test triggered")
  global vm
  vm.writeline("Here is a test\n")


vm.on_output("trigger_test_serial_port", test_serial_port)

# Boot the VM
vm.make().boot(80)
