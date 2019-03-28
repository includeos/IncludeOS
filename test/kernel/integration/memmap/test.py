#! /usr/bin/env python

from __future__ import print_function
from builtins import str
import sys
import os

from vmrunner import vmrunner

image_name="build/service"
if len(sys.argv) > 1:
    image_name=str(sys.argv[1])

def test2():
  print("Booting VM 2 - lots of memory")
  vm = vmrunner.vm(config = "vm2.json")
  vm.boot(20, image_name = image_name)

vm = vmrunner.vm(config = "vm1.json")
vm.on_exit_success(test2)
print("Booting VM 1 - default amount of memory")

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(20,image_name='kernel_memmap').clean()
