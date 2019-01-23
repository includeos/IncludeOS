#! /usr/bin/env python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner

image_name="build/service"
if len(sys.argv) > 1:
    image_name=str(sys.argv[1])

def test2():
  print "Booting VM 2 - lots of memory"
  vm = vmrunner.vm(config = "vm2.json")
  vm.boot(20, image_name = image_name)

vm = vmrunner.vm(config = "vm1.json")
vm.on_exit_success(test2)
print "Booting VM 1 - default amount of memory"

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(20,image_name='kernel_memmap').clean()
