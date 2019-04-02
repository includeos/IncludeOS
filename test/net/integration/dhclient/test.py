#! /usr/bin/env python

from __future__ import print_function
from builtins import str
import sys
import os
import time
import subprocess
import subprocess32

thread_timeout = 20

from vmrunner import vmrunner
import socket

from vmrunner.prettify import color

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

ping_count = 3

def DHCP_test(trigger_line):
  print(color.INFO("<Test.py>"),"Got IP")
  ip_string = vm.readline()
  print(color.INFO("<Test.py>"), "Assigned address: ", ip_string)
  print(color.INFO("<Test.py>"), "Trying to ping")
  time.sleep(1)
  try:
    command = ["ping", ip_string.rstrip(), "-c", str(ping_count), "-i", "0.2"]
    print(color.DATA(" ".join(command)))
    print(subprocess32.check_output(command, timeout=thread_timeout))
    vm.exit(0,"<Test.py> Ping test passed. Process returned 0 exit status")
  except Exception as e:
    print(color.FAIL("<Test.py> Ping FAILED Process threw exception:"))
    print(e)
    return False

# Add custom event-handler
vm.on_output("Got IP from DHCP", DHCP_test)

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vmrunner.vms[0].boot(image_name=str(sys.argv[1]))
else:
    vmrunner.vms[0].cmake().boot(thread_timeout,image_name='net_dhclient').clean()
