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

S_HOST, S_PORT = '', 4242
S_MESSAGE = "Only hipsters uses POSIX"
server = socket.socket
server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

server.bind((S_HOST, S_PORT))

def UDP_send(trigger_line):
  HOST, PORT = '10.0.0.45', 1042
  MESSAGE = "POSIX is for hipsters"
  sock = socket.socket
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  sock.sendto(MESSAGE, (HOST, PORT))

def UDP_recv(trigger_line):
  received = server.recv(1024)
  return received == S_MESSAGE

def UDP_send_much(trigger_line):
  HOST, PORT = '10.0.0.45', 1042
  MESSAGE = "Message #"
  sock = socket.socket
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  sock.connect((HOST, PORT))

  for i in range(0, 5):
    sock.send(MESSAGE + `i`)
    print "Sending", MESSAGE + `i`

# Add custom event-handler
vm.on_output("recvfrom()", UDP_send)
vm.on_output("sendto()", UDP_recv)
vm.on_output("send() and connect()", UDP_recv)
vm.on_output("reading from buffer", UDP_send_much)

# Boot the VM, taking a timeout as parameter
vm.make().boot(10)

