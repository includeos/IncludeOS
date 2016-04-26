#! /usr/bin/python
import sys
sys.path.insert(0,"..")

import vmrunner
import socket

HOST, PORT = "10.0.0.42", 4242
# SOCK_DGRAM is the socket type to use for UDP sockets
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]


def UDP_test():
  print "<Test.py> Performing UDP test 1"

  data = "Douche"
  sock.sendto(data+"\n", (HOST, PORT))
  received = sock.recv(100)

  print "<Test.py> Sent:     {}".format(data)
  print "<Test.py> Received: {}".format(received)

  prev = -1
  OK = False

  while not vm.poll():
    received = sock.recv(1500)
    nr = int(received.split(" ")[1]);

    if nr != prev + 1:
      break

    print "Got packet ", nr

    prev += 1
    if nr == 599:
      OK = True
      break

  if OK:
    print "<test.py> SUCCESS - correct packet order"
    sock.sendto("SUCCESS", (HOST, PORT))
  else:
    print "<test.py> FAILED - incorrect packet order"

# Add custom event-handler
vm.on_output("Done. Send some UDP-data", UDP_test)

# Boot the VM, taking a timeout as parameter
vm.boot(20)
