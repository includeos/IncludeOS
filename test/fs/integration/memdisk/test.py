#! /usr/bin/env python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from subprocess import call

from vmrunner import vmrunner
vm = vmrunner.vms[0]

# Make default (nosector) and boot the VM
vm.cmake().boot(20).clean()
