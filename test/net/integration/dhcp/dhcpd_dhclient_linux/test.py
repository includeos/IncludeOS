#! /usr/bin/env python

import sys
import os
import time
import subprocess

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
import socket

from vmrunner.prettify import color

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

num_messages = 0
ping_passed = False
ping_count = 3

def check_dhclient_output(output):
  global num_messages

  if "DHCPDISCOVER on bridge43 to 255.255.255.255 port 67" in output:
    num_messages += 1

  if "DHCPOFFER of 10.200.100.20 from 10.200.0.1" in output:
    num_messages += 1

  if "DHCPREQUEST of 10.200.100.20 on bridge43 to 255.255.255.255 port 67" in output:
    num_messages += 1

  if "DHCPACK of 10.200.100.20 from 10.200.0.1" in output:
    num_messages += 1

  if num_messages != 4:
    print color.FAIL("<Test.py> DHCP process FAILED")

def ping_test(ip):
  global ping_passed
  print color.INFO("<Test.py>"), "Assigned address: ", ip
  print color.INFO("<Test.py>"), "Trying to ping"
  time.sleep(1)
  try:
    command = ["ping", ip, "-c", str(ping_count), "-i", "0.2"]
    print color.DATA(" ".join(command))
    print subprocess.check_output(command)
    ping_passed = True
  except Exception as e:
    print color.FAIL("<Test.py> Ping FAILED Process threw exception:")
    print e
    return False

def run_dhclient(trigger_line):
  route_output = subprocess.check_output(["route"])

  if "10.200.0.0" not in route_output:
    subprocess.call(["route", "add", "-net", "10.200.0.0", "netmask", "255.255.0.0", "dev", "bridge43"])
    print color.INFO("<Test.py>"), "Route added to bridge43, 10.200.0.0"

  print color.INFO("<Test.py>"), "Running dhclient"

  try:
    dhclient = subprocess.Popen(
        ["dhclient", "bridge43", "-4", "-n", "-v"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    process_output, _ = dhclient.communicate()
    print color.INFO("<dhclient>")
    print process_output
    check_dhclient_output(process_output)
  except (OSError, subprocess.CalledProcessError) as exception:
    print color.FAIL("<Test.py> dhclient FAILED threw exception:")
    print str(exception)
    return False

  ping_test("10.200.100.20")

  # Remove leases-file
  print color.INFO("<Test.py>"), "Removing /var/lib/dhcp/dhclient.leases"
  subprocess.call(["rm", "/var/lib/dhcp/dhclient.leases"])

  if ping_passed and num_messages == 4:
    vm.exit(0,"<Test.py> DHCP process and ping test completed successfully. Process returned 0 exit status")

# Add custom event-handler
vm.on_output("Service started", run_dhclient)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(20).clean()
