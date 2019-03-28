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
from vmrunner.prettify import color

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

num_assigned_clients = 0
ping_count = 3

def DHCP_test(trigger_line):
  global num_assigned_clients
  num_assigned_clients += 1
  print(color.INFO("<Test.py>"),"Client got IP")
  ip_string = vm.readline()
  print(color.INFO("<Test.py>"), "Assigned address: ", ip_string)
  print(color.INFO("<Test.py>"), "Trying to ping")
  time.sleep(1)
  try:
    command = ["ping", "-c", str(ping_count), "-i", "0.2", ip_string.rstrip()]
    print(color.DATA(" ".join(command)))
    print(subprocess32.check_output(command, timeout=thread_timeout))
    print(color.INFO("<Test.py>"), "Number of ping tests passed: ", str(num_assigned_clients))
    if num_assigned_clients == 3:
      vm.exit(0,"<Test.py> Ping test for all 3 clients passed. Process returned 0 exit status")
  except Exception as e:
    print(color.FAIL("<Test.py> Ping FAILED Process threw exception:"))
    print(e)
    return False

# Add custom event-handler
vm.on_output("Client 1 got IP from IncludeOS DHCP server", DHCP_test)
vm.on_output("Client 2 got IP from IncludeOS DHCP server", DHCP_test)
vm.on_output("Client 3 got IP from IncludeOS DHCP server", DHCP_test)

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vmrunner.vms[0].boot(image_name=str(sys.argv[1]))
else:
    vmrunner.vms[0].cmake().boot(thread_timeout,image_name='net_dhcpd').clean()
