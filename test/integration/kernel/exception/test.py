#!/usr/bin/env python3
from builtins import str
import sys
import os
import socket

from vmrunner import vmrunner
vm = vmrunner.vms[0]

counter = 0
expected = 5
def is_good(line):
    global counter
    counter += 1
    print("<Test.py> Found expected line {}/{}\n".format(counter, expected, line))

    if (counter == expected):
        vm.exit(0, "All tests passed")

vm.on_output("\\x15\\x07\\t\*\*\*\* PANIC \*\*\*\*", is_good)
vm.on_output("Divide-by-zero Error", is_good)

vm.on_output("__cpu_exception", is_good)
vm.on_output("Service::start()", is_good)

vm.boot(20,image_name='kernel_exception')
