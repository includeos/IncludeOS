#!/usr/bin/python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src + "/test")

import vmrunner
vm = vmrunner.vms[0]

num_outputs = 0

def increment(line):
  global num_outputs
  num_outputs += 1
  print "num_outputs after increment: ", num_outputs

def check_num_outputs(line):
  assert(num_outputs == 2)
  vmrunner.vms[0].exit(0, "SUCCESS")

vm.on_output("chdir result: -1", increment)
vm.on_output("Old umask: 2", increment)

vm.on_output("All done!", check_num_outputs)

# Boot the VM, taking a timeout as parameter
vm.boot(20)
