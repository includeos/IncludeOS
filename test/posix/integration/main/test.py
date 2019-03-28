#! /usr/bin/env python
from __future__ import print_function
from builtins import str
import sys
import os

from vmrunner import vmrunner
from vmrunner.prettify import color

vm = vmrunner.vms[0]

N = 2
T = 0

def check_exit(line, n = "0"):
    global T
    T += 1
    print(color.INFO("test.py"), "received: ", line)
    status = line.split(" ")[-1].lstrip().rstrip()
    as_expected = status == n

    if as_expected:
        print(color.INFO("test.py"), "Exit status is ", status, "as expected")
        vm.exit(0, "Test " + str(T) + "/" + str(N) + " passed", keep_running = True)
        return as_expected
    else:
        print(color.WARNING("test.py"), "Exit status is", status, "expected", n)
        return as_expected

def exit1(line):
    return check_exit(line, "200")

def exit2(line):
    return check_exit(line, "0")

vm.on_output("returned with status", exit1)

if len(sys.argv) > 1:
    vm.boot(30,image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(30).cmake(["-DNORMAL=OFF"]).on_output("returned with status", exit2).boot(image_name='posix_main').clean()
