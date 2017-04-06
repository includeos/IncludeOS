#! /usr/bin/env python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner

vm = vmrunner.vms[0];

chainloaded = False


def chainload_ok(string):
    global chainloaded
    chainloaded = True
    return chainloaded


def verify_chainload():
    print "<test.py>", "Chainloaded: ", chainloaded
    return chainloaded

vm.on_output("IncludeOS was just chainloaded", chainload_ok);
vm.on_exit(verify_chainload)

# Build, run and clean
vm.cmake().boot().clean()
