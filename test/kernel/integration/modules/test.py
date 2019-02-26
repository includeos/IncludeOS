#! /usr/bin/env python

import sys
import os
import subprocess

HOST="10.0.0.53"
includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner

vm = vmrunner.vms[0];

chainloaded = False
pinged = False

def chainload_ok(string):
    global chainloaded
    global pinged
    chainloaded = True
    pinged = subprocess.check_call(["sudo","ping", HOST, "-c", "3"]) == 0;
    if pinged:
        vm.exit(0,"Chainloaded vm up and answered ping")
    return chainloaded and pinged


def verify_chainload():
    print "<test.py>", "Chainloaded: ", chainloaded, "trying to ping"
    return chainloaded and pinged == 0

vm.on_output("IncludeOS was just chainloaded", chainload_ok);
vm.on_exit(verify_chainload)

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Build, run and clean
    vm.cmake().boot(image_name='kernel_modules').clean()
