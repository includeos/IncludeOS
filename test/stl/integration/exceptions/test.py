#! /usr/bin/env python

from __future__ import print_function
from builtins import str
import sys
import os

from vmrunner import vmrunner

vm = vmrunner.vms[0]

tests_ok = 0

def test_ok(line):
    global tests_ok
    tests_ok += 1
    if (tests_ok == 2):
        vm.exit(0, "All tests passed")

def expected_panic(line):
    print("<test.py> VM panicked")
    if (tests_ok == 1):
        return True
    else:
        return False

def test_fail(line):
    print("Test didn't get expected panic output before end of backtrace")
    return False

vm.on_output("Part 1 OK", test_ok)
vm.on_panic(expected_panic, False)
vm.on_output("Uncaught exception expecting panic", test_ok)
vm.on_output("long_mode", test_fail)

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    #the corutines is set in the CMakelists.
    vm.cmake().boot(30,image_name='stl_exceptions').clean()
