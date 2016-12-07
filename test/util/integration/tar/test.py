#! /usr/bin/env python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src + "/test")

import vmrunner
vm = vmrunner.vms[0]

num_elements = 0

#num_ = 0

def increment_element(line):
  global num_elements
  num_elements += 1
  print "num_elements after increment: ", num_elements

def check_num_outputs(line):
  assert(num_elements == 11)
  #assert(num_outputs == 53)
  vmrunner.vms[0].exit(0, "SUCCESS")

# All elements in tarball
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/ ", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/l1_f1/ ", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/l1_f1/l2/ ", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/l1_f1/l2/README.md", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/l1_f1/l2/service.cpp", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/l1_f1/service.cpp", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/l1_f1/Makefile", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/second_file.md", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/first_file.txt", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/l1_f2/ ", increment_element)
vm.on_output("Element name: home/annikaha/IncludeOS/test/util/integration/tar/tar_example/l1_f2/virtio.hpp", increment_element)

#vm.on_output("", increment_header1)

vm.on_output("Something special to close with", check_num_outputs)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(20).clean()
