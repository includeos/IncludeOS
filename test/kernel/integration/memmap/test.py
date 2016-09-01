#! /usr/bin/python

import sys
import os

includeos_src = os.environ['INCLUDEOS_SRC']
sys.path.insert(0,includeos_src + "/test")

import vmrunner


def test2():
  print "Booting VM 2 - lots of memory";
  vmrunner.vms[1].boot(20)

vm = vmrunner.vms[0];
vm.make().on_exit_success(test2);
print "Booting VM 1 - default amount of memory"
vm.boot(20)
