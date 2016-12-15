#! /usr/bin/env python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
vm = vmrunner.vms[0]
vm.cmake()

num_outputs = 0

def increment(line):
  global num_outputs
  num_outputs += 1
  print "num_outputs after increment: ", num_outputs

def check_num_outputs(line):
  assert(num_outputs == 5)
  vmrunner.vms[0].exit(0, "SUCCESS")

vm.on_output("Sysname: IncludeOS", increment)
vm.on_output("Nodename: IncludeOS-node", increment)
vm.on_output("Release: v", increment)
vm.on_output("Version: v", increment)
vm.on_output("Machine: x86_64", increment)
vm.on_output("Something special to close with", check_num_outputs)

# Boot the VM, taking a timeout as parameter
vm.boot(20).clean()
