#! /usr/bin/env python

from __future__ import print_function
from builtins import str
import sys
import os
import time
import subprocess
import subprocess32

thread_timeout = 20

from threading import Timer

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
import socket

from vmrunner.prettify import color

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

assigned_ip = ""
num_messages = 0
ping_passed = False
ping_count = 3

def cleanup():
  # Remove leases-file
  print(color.INFO("<Test.py>"), "Removing /var/lib/dhcp/dhclient.leases")
  subprocess.call(["sudo", "rm", "/var/lib/dhcp/dhclient.leases"])
  # Kill dhclient process:
  subprocess.call(["sudo", "dhclient", "bridge43", "-4", "-x", "-n", "-v"])

def check_dhclient_output(output):
  global num_messages
  global assigned_ip

  ip_length = 13
  ip_index = output.find("10.200.100.")

  if ip_index != -1:
    assigned_ip = output[ip_index : ip_index + ip_length]
  else:
    cleanup()
    vm.exit(1, "<Test.py> IP not found in output from dhclient")

  if "DHCPDISCOVER on bridge43 to 255.255.255.255 port 67" in output:
    num_messages += 1

  if "DHCPOFFER of " + assigned_ip + " from 10.200.0.1" in output:
    num_messages += 1

  if "DHCPREQUEST of " + assigned_ip + " on bridge43 to 255.255.255.255 port 67" in output:
    num_messages += 1

  if "DHCPACK of " + assigned_ip + " from 10.200.0.1" in output:
    num_messages += 1

  if num_messages != 4:
    cleanup()
    vm.exit(1, "<Test.py> DHCP process FAILED")

def ping_test():
  global ping_passed

  print(color.INFO("<Test.py>"), "Assigned address: ", assigned_ip)
  print(color.INFO("<Test.py>"), "Trying to ping")
  time.sleep(1)
  try:
    command = ["ping", assigned_ip, "-c", str(ping_count), "-i", "0.2"]
    print(color.DATA(" ".join(command)))
    print(subprocess32.check_output(command, timeout=thread_timeout))
    ping_passed = True
  except Exception as e:
    print(color.FAIL("<Test.py> Ping FAILED Process threw exception:"))
    print(e)
    cleanup()
    vm.exit(1, "<Test.py> Ping test failed")
  finally:
    cleanup()

def run_dhclient(trigger_line):
  route_output = subprocess.check_output(["route"])

  if "10.0.0.0" not in route_output:
    subprocess32.call(["sudo", "ifconfig", "bridge43", "10.0.0.1", "netmask", "255.255.0.0", "up"], timeout=thread_timeout)
    time.sleep(1)

  if "10.200.0.0" not in route_output:
    subprocess32.call(["sudo", "route", "add", "-net", "10.200.0.0", "netmask", "255.255.0.0", "dev", "bridge43"], timeout=thread_timeout)
    print(color.INFO("<Test.py>"), "Route added to bridge43, 10.200.0.0")

  print(color.INFO("<Test.py>"), "Running dhclient")

  try:
    dhclient = subprocess32.check_output(
        ["sudo", "dhclient", "bridge43", "-4", "-n", "-v"],
        stderr=subprocess.STDOUT,
        timeout=thread_timeout
    )

    print(color.INFO("<dhclient>"))
    print(dhclient)

    # gets ip of dhclient used to ping
    check_dhclient_output(dhclient)

  except subprocess.CalledProcessError as exception:
    print(color.FAIL("<Test.py> dhclient FAILED threw exception:"))
    print(exception.output)
    vm.exit(1, "<Test.py> dhclient test failed")
    return False

  ping_test()

  if ping_passed and num_messages == 4:
    vm.exit(0, "<Test.py> DHCP process and ping test completed successfully. Process returned 0 exit status")
  else:
    vm.exit(1, "<Test.py> DHCP process or ping test failed")

# Add custom event-handler
vm.on_output("Service started", run_dhclient)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(thread_timeout).clean()
