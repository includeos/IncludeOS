#!/usr/bin/env python3

from builtins import str
import sys
import os

from vmrunner import vmrunner

vm = vmrunner.vms[0]

counter = 0
expected = 4
def is_good(line):
    global counter, expected
    counter += 1
    print("<test.py> Found expected line {}/{}\n".format(counter, expected, line))
    if (counter == expected):
        vm.exit(0, "All tests passed")

vm.on_output("Service::start()", is_good)
vm.on_output("kernel_main", is_good)
vm.on_output("libc_start_main", is_good)
vm.on_output("long_mode", is_good)

vm.boot(20,image_name='kernel_stacktrace')
