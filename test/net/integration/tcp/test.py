#! /usr/bin/env python

import socket
import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
from vmrunner.prettify import color

# Usage: python test.py $GUEST_IP $HOST_IP
GUEST = '10.0.0.44' if (len(sys.argv) < 2) else sys.argv[1]
HOST = '10.0.0.1' if (len(sys.argv) < 3) else sys.argv[2]


TEST1 = 8081
TEST2 = 8082
TEST3 = 8083
TEST4 = 8084
TEST5 = 8085

INFO = color.INFO("<test.py>")

def connect(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = (GUEST, port)
    print INFO, 'connecting to %s port %s' % server_address
    sock.connect(server_address)
    bytes_received = 0
    try:
        while True:
            data = sock.recv(1024)
            bytes_received += len(data)
            #print >>sys.stderr, '%s' % data
            if data:
                sock.sendall(data);
            else:
                break
    finally:
        print INFO, 'closing socket. Received ', str(bytes_received),"bytes"
        sock.close()

    return True

def listen(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_address = (HOST, port)
    print INFO, 'starting up on %s port %s' % server_address
    sock.bind(server_address)
    sock.listen(1)

    while True:
        connection, client_address = sock.accept()
        try:
            print INFO, 'connection from', client_address
            while True:
                data = connection.recv(1024)
                if data:
                    print INFO,'received data, sending data back to the client'
                    connection.sendall(data)
                    print INFO,'close connection to client'
                    connection.close()
                else:
                    print INFO,'no more data from', client_address
                    break

        finally:
            connection.close()
            break

        sock.close()
        return True


def test1(trigger):
    print INFO, trigger.rstrip(),  "triggered by VM"
    return connect(TEST1)

def test2(trigger):
    print INFO, trigger.rstrip(),  "triggered by VM"
    return connect(TEST2)

def test3(trigger):
    print INFO, trigger.rstrip(),  "triggered by VM"
    return connect(TEST3)

def test4(trigger):
    print INFO, trigger.rstrip(),  "triggered by VM"
    connect(TEST4)

def test5(trigger):
    print INFO, trigger.rstrip(),  "triggered by VM"
    listen(TEST5)

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event-handler
vm.on_output("TEST1", test1)
vm.on_output("TEST2", test2)
vm.on_output("TEST3", test3)
vm.on_output("TEST4", test4)
vm.on_output("TEST5", test5)


# Boot the VM, taking a timeout as parameter
vm.cmake().boot(120).clean()
