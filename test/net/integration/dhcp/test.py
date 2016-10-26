#! /usr/bin/python

import sys
import os
import time
import subprocess

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src + "/test")

import vmrunner
import socket

from prettify import color

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

ping_count = 3

def DHCP_test(trigger_line):
  print "<Test.py> Got IP"
  ip_string = vm.readline()
  print "<Test.py> Assigned address: ", ip_string
  print "<Test.py> Trying to ping"
  time.sleep(1)
  try:
    command = ["ping", ip_string.rstrip(), "-c", str(ping_count), "-i", "0.2"]
    print color.DATA(" ".join(command))
    print subprocess.check_output(command)
    print "<Test.py> Ping SUCCESS - Process returned 0 exit status"
    vm.exit(0,"SUCCESS")
  except Exception as e:
    print "<Test.py> Ping FAILED Process threw exception:"
    print e
    return False


# Add custom event-handler
vm.on_output("Got IP from DHCP", DHCP_test)

# Boot the VM, taking a timeout as parameter
vm.make().boot(20)
