#! /usr/bin/python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src + "/test")

import vmrunner
import socket

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

def UDP_test(trigger_line):
  print "<Test.py> Performing UDP tests"
  HOST, PORT = "10.0.0.45", 4242
  sock = socket.socket
  # SOCK_DGRAM is the socket type to use for UDP sockets
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  data = "Douche"
  sock.sendto(data, (HOST, PORT))
  received = sock.recv(1024)

  print "<Test.py> Sent:     {}".format(data)
  print "<Test.py> Received: {}".format(received)
  if received != data: return False

  data = "Bag"
  sock.sendto(data, (HOST, PORT))
  received = sock.recv(1024)

  print "<Test.py> Sent:     {}".format(data)
  print "<Test.py> Received: {}".format(received)
  if received == data:
    vmrunner.vms[0].exit(0, "SUCCESS")

# Add custom event-handler
vm.on_output("IncludeOS UDP test", UDP_test)

# Boot the VM, taking a timeout as parameter
vm.make().boot(20)
