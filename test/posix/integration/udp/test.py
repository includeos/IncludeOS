#! /usr/bin/env python

import sys
import os
import subprocess
import atexit

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
vm = vmrunner.vms[0]

import socket

# Set up a temporary interface
import platform
if platform.system() == 'Darwin':
    subprocess.call(["sudo", "ifconfig", "bridge43", "alias", "10.0.0.3/24"])
else:
    subprocess.call(["sudo", "ifconfig", "bridge43:1", "10.0.0.3/24"])

# Tear down interface on exit
@atexit.register
def tear_down():
    if platform.system() == 'Darwin':
        subprocess.call(["sudo", "ifconfig", "bridge43", "-alias", "10.0.0.3"])
    else:
        subprocess.call(["sudo", "ifconfig", "bridge43:1", "down"])


S_HOST, S_PORT = '10.0.0.3', 4242
S_MESSAGE = "Only hipsters uses POSIX"
server = socket.socket
server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


server.bind((S_HOST, S_PORT))

HOST, PORT = '10.0.0.58', 1042

RECEIVED = ''

def UDP_send(trigger_line):
  MESSAGE = "POSIX is for hipsters"
  sock = socket.socket
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.bind((S_HOST, S_PORT + 1))
  sock.sendto(MESSAGE, (HOST, PORT))

def UDP_recv():
  RECEIVED = server.recv(1024)

def verify_recv(trigger_line):
  ok = RECEIVED == S_MESSAGE
  RECEIVED = ''
  return ok

def UDP_send_much(trigger_line):
  MESSAGE = "Message #"
  sock = socket.socket
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  sock.connect((HOST, PORT))

  for i in range(0, 5):
    sock.send(MESSAGE + `i`)
    print "Sending", MESSAGE + `i`

import thread
thread.start_new_thread(UDP_recv, ())

# Add custom event-handler
vm.on_output("recvfrom()", UDP_send)
vm.on_output("sendto() called", verify_recv)
vm.on_output("sendto() called", verify_recv)
vm.on_output("reading from buffer", UDP_send_much)

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(10,image_name='posix_udp').clean()
