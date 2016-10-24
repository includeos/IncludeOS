#! /usr/bin/python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])

sys.path.insert(0,includeos_src + "/test")

import vmrunner
from prettify import color

vm = vmrunner.vms[0]

N = 2
T = 0

def check_exit(line, n = "0"):
    global T
    T += 1
    print "Python received: ", line
    status = line.split(" ")[-1].lstrip().rstrip()
    as_expected = status == n
    print "Exit status is ", status, "as expected" if as_expected else "expected " + n
    if as_expected: vm.exit(0, color.SUCCESS("Test " + str(T) + "/" + str(N) + " passed"))
    else: return as_expected



def exit1(line):
    return check_exit(line, "200")

def exit2(line):
    return check_exit(line, "0")

def main_no_params():
    print "VM exited. Restarting."
    vm.on_output("Hello main", lambda(line): True)
    vm.on_output("returned with status", exit2)
    vm.on_exit(lambda: 0)
    vm.make(["FILES=main_no_params.cpp"]).boot()

# Default test (main with params) - check for exit value (printed by weak Service::start for now)
vm.on_output("returned with status", exit1)

# After the default test (main with params) do the second test
vm.on_exit(main_no_params)
vm.make().boot(40)
