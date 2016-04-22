#! /usr/bin/python
import sys
sys.path.insert(0,"..")

import vmrunner
import socket


def UDP_test():
  print "<Test.py> Performing UDP tests"
  HOST, PORT = "10.0.0.42", 4242
  sock = socket.socket
  # SOCK_DGRAM is the socket type to use for UDP sockets
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  data = "Douche"
  sock.sendto(data+"\n", (HOST, PORT))
  received = sock.recv(1024)

  print "<Test.py> Sent:     {}".format(data)
  print "<Test.py> Received: {}".format(received)

  data = "Bag"
  sock.sendto(data+"\n", (HOST, PORT))
  received = sock.recv(1024)

  print "<Test.py> Sent:     {}".format(data)
  print "<Test.py> Received: {}".format(received)


# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event-handler
vm.on_output("IncludeOS UDP test", UDP_test)

# Boot the VM, taking a timeout as parameter
vm.boot(20)
