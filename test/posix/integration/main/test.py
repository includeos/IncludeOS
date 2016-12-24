#! /usr/bin/env python
import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
    os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
from vmrunner.prettify import color

vm = vmrunner.vms[0]

N = 2
T = 0

def check_exit(line, n = "0"):
    global T
    T += 1
    print "Python received: ", line
    status = line.split(" ")[-1].lstrip().rstrip()
    as_expected = status == n

    if as_expected:
        print color.INFO("test.py"), "Exit status is ", status, "as expected"
        vm.exit(0, "Test " + str(T) + "/" + str(N) + " passed")
        return as_expected
    else:
        print color.WARNING("test.py"), "Exit status is", status, "expected", as_expected
        "expected " + n
        return as_expected

def exit1(line):
    return check_exit(line, "200")

def exit2(line):
    return check_exit(line, "0")

def main_no_params():
    print "VM exited. Restarting."
    vm.clean()

    # NOTE:
    # We can't add more output functions when reusing the same VM object
    # This will call python to complain about dictionary being resized while iterating
    # e.g. this would fail:
    # vm.on_output("Hello main", lambda(line): True)

    # overwrite the main returned event
    vm.on_output("returned with status", exit2)

    # Overwrite the on_exit event to avoid infinite loop
    vm.on_exit(lambda: 0)

    # Build and run the second version of the service
    vm.cmake(["-DNORMAL=OFF"]).boot().clean()

# Default test (main with params) - check for exit value (printed by weak Service::start for now)
vm.on_output("returned with status", exit1)

# After the default test (main with params) do the second test
vm.on_exit(main_no_params)
vm.cmake().boot(30)
