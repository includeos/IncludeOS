#! /usr/bin/env python

from __future__ import print_function
from builtins import str
from builtins import range
import sys
import os
import subprocess

from vmrunner import vmrunner
from vmrunner.prettify import color

import socket

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

num_successes = 0

def start_icmp_test(trigger_line):
  global num_successes

  # 1 Ping: Checking output from callback in service.cpp
  print(color.INFO("<Test.py>"), "Performing ping6 test")

  output_data = ""
  for x in range(0, 9):
    output_data += vm.readline()

  print(output_data)

  if "Received packet from gateway" in output_data and \
    "Identifier: 0" in output_data and \
    "Sequence number: 0" in output_data and \
    "Source: fe80:0:0:0:e823:fcff:fef4:83e7" in output_data and \
    "Destination: fe80:0:0:0:e823:fcff:fef4:85bd" in output_data and \
    "Type: ECHO REPLY (129)" in output_data and \
    "Code: DEFAULT (0)" in output_data and \
    "Checksum: " in output_data and \
    "Data: INCLUDEOS12345ABCDEFGHIJKLMNOPQRSTUVWXYZ12345678" in output_data:
    num_successes += 1
    print(color.INFO("<Test.py>"), "Ping test succeeded")
  else:
    print(color.FAIL("<Test.py>"), "Ping test FAILED")
    vm.exit(1, "Ping test failed")

  if num_successes == 1:
    vm.exit(0, "<Test.py> All ICMP tests succeeded. Process returned 0 exit status")

vm.on_output("Service IPv4 address: 10.0.0.52, IPv6 address: fe80:0:0:0:e823:fcff:fef4:85bd", start_icmp_test);

if len(sys.argv) > 1:
    vm.boot(50,image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.cmake().boot(50,image_name='net_icmp6').clean()
