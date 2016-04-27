#! /usr/bin/python
import sys
sys.path.insert(0,"..")

import vmrunner
import socket
import time

HOST, PORT = "10.0.0.42", 4242
# SOCK_DGRAM is the socket type to use for UDP sockets
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]


packet_content = True
def assert_packet_content(test):
  if not test:
    packet_content = False
    print "<Test.py> FAIL: Packet content is wrong"
  return test

packet_order = True
def assert_packet_order(test):
  if not test:
    packet_order = False
    print "<Test.py> FAIL: Packet order is wrong"
  return test

def UDP_test():
  print "<Test.py> Performing UDP test 1"

  data = "Douche"
  sock.sendto(data+"\n", (HOST, PORT))
  received = sock.recv(100)

  print "<Test.py> Sent:     {}".format(data)
  print "<Test.py> Received: {}".format(received)

  expect = "A"

  packet_size = 1472
  packet_count = 2400

  OK = False
  i = 1
  bytes_received = 0;

  t1 = time.clock()

  while not vm.poll():
    received = sock.recv(65000)
    bytes_received += len(received)
    first_char = received[0]
    last_char = received[len(received)-1]
    #print "Packet",i,len(received),"bytes, (",bytes_received," total)",first_char, " - ", last_char

    if not assert_packet_content(first_char == last_char): return False
    if not assert_packet_order(ord(first_char) == ord(expect)): return False

    # Make sure the chars in this packets were incremented or wrapped around
    if first_char == 'Z' : expect = "A"
    else : expect = chr(ord(first_char) + 1)
    i += 1

    if (bytes_received >= packet_count * packet_size):
      break

  time_taken = time.clock() - t1
  print "<test.py> Transmission finished in ", time_taken

  if packet_order and packet_content:
    sock.sendto("SUCCESS", (HOST, PORT))

  return packet_order and packet_content


# Add custom event-handler
vm.on_output("Done. Send some UDP-data", UDP_test)

# Boot the VM, taking a timeout as parameter
vm.boot(20)
