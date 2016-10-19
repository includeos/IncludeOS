#!/usr/bin/python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src + "/test")

import vmrunner
vm = vmrunner.vms[0]

import socket

def UDP_send():
  HOST, PORT = '10.0.0.45', 42
  MESSAGE = "POSIX is for hipsters"
  sock = socket.socket
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  sock.sendto(MESSAGE, (HOST, PORT))

def UDP_recv():
  HOST, PORT = '', 4242
  MESSAGE = "Only hipsters uses POSIX"
  sock = socket.socket
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  sock.bind(('', PORT))

  received = sock.recv(1024)
  return received == MESSAGE

# Add custom event-handler
vm.on_output("recvfrom()", UDP_send)
vm.on_output("sendto()", UDP_recv)

# Boot the VM, taking a timeout as parameter
vm.make().boot(10)
