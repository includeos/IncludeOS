#! /usr/bin/env python

from __future__ import print_function
from future import standard_library
standard_library.install_aliases()
from builtins import str
import sys
import os
import subprocess
import atexit

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print('includeos_src: {0}'.format(includeos_src))
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
vm = vmrunner.vms[0]

import socket

# Set up a temporary interface
import platform
if platform.system() == 'Darwin':
    subprocess.call(["sudo", "ifconfig", "bridge43", "alias", "10.0.0.4/24"])
else:
    subprocess.call(["sudo", "ifconfig", "bridge43:2", "10.0.0.4/24"])

# Tear down interface on exit
@atexit.register
def tear_down():
    if platform.system() == 'Darwin':
        subprocess.call(["sudo", "ifconfig", "bridge43", "-alias", "10.0.0.4"])
    else:
        subprocess.call(["sudo", "ifconfig", "bridge43:2", "down"])


S_HOST, S_PORT = '10.0.0.4', 4242
S_MESSAGE = "Only hipsters uses POSIX"
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((S_HOST, S_PORT))

HOST, PORT = '10.0.0.57', 1042

RECEIVED = ''

def verify_recv(recv):
  ok = recv == S_MESSAGE
  return ok

def TCP_connect():
  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  sock.connect((HOST, PORT))
  MESSAGE = "POSIX is for hipsters"
  sock.send(MESSAGE)
  sock.close()

def TCP_recv(trigger_line):
  server.listen(1)
  conn, addr = server.accept()
  RECEIVED = conn.recv(1024)
  conn.close()
  return verify_recv(RECEIVED)

import _thread
def TCP_connect_thread(trigger_line):
  _thread.start_new_thread(TCP_connect, ())

# Add custom event-handler
vm.on_output("accept()", TCP_connect_thread)
vm.on_output("Trigger TCP_recv", TCP_recv)

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(20,image_name='posix_tcp').clean()
