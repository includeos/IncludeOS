#! /usr/bin/env python

import sys
import os
import time
import subprocess

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
from vmrunner.prettify import color

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

def DNS_test(trigger_line):
  dns = ""

# Add custom event-handler
vm.on_output("", DNS_test)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(20).clean()
