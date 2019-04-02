#! /usr/bin/env python

from __future__ import print_function
from builtins import str
import sys
import os
import time
import subprocess

from vmrunner import vmrunner
import socket

from vmrunner.prettify import color

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

ping_count = 3

def Slaac_test(trigger_line):
  print(color.INFO("<Test.py>"),"Got IP")
  vm_string = vm.readline()
  wlist = vm_string.split()
  ip_string = wlist[-1].split("/")[0]
  print(color.INFO("<Test.py>"), "Assigned address: ", ip_string)
  print(color.INFO("<Test.py>"), "Trying to ping")
  time.sleep(1)
  try:
    command = ["ping6", "-I", "bridge43", ip_string.rstrip(), "-c",
            str(ping_count) ]
    print(color.DATA(" ".join(command)))
    print(subprocess.check_output(command))
    vm.exit(0,"<Test.py> Ping test passed. Process returned 0 exit status")
  except Exception as e:
    print(color.FAIL("<Test.py> Ping FAILED Process threw exception:"))
    print(e)
    return False


# Add custom event-handler
vm.on_output("Waiting for Auto-configuration", Slaac_test)

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.cmake().boot(20,image_name='net_slaac').clean()
