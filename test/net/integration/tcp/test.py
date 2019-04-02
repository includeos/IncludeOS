#! /usr/bin/env python

from __future__ import print_function
from builtins import str
import socket
import sys
import os

from vmrunner import vmrunner
from vmrunner.prettify import color

GUEST = 'fe80:0:0:0:e823:fcff:fef4:85bd%bridge43'
HOST = 'fe80:0:0:0:e823:fcff:fef4:83e7%bridge43'


TEST1 = 8081
TEST2 = 8082
TEST3 = 8083
TEST4 = 8084
TEST5 = 8085

INFO = color.INFO("<test.py>")

def connect(port):
    addr = (GUEST, port)
    res = socket.getaddrinfo(addr[0], addr[1], socket.AF_INET6, socket.SOCK_STREAM, socket.SOL_TCP)
    af, socktype, proto, canonname, sa = res[0]
    sock = socket.socket(af, socktype, proto)
    print(INFO, 'connecting to %s' % res)
    sock.connect(sa)
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
        print(INFO, 'closing socket. Received ', str(bytes_received),"bytes")
        sock.close()

    return True

def listen(port):
    addr = (HOST, port)
    res = socket.getaddrinfo(addr[0], addr[1], socket.AF_INET6, socket.SOCK_STREAM, socket.SOL_TCP)
    print(INFO, 'starting up on %s' % res)
    af, socktype, proto, canonname, sa = res[0]
    sock = socket.socket(af, socktype, proto)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(sa)
    sock.listen(1)

    while True:
        connection, client_address = sock.accept()
        try:
            print(INFO, 'connection from', client_address)
            while True:
                data = connection.recv(1024)
                if data:
                    print(INFO,'received data, sending data back to the client')
                    connection.sendall(data)
                    print(INFO,'close connection to client')
                    connection.close()
                else:
                    print(INFO,'no more data from', client_address)
                    break

        finally:
            connection.close()
            break

        sock.close()
        return True


def test1(trigger):
    print(INFO, trigger.rstrip(),  "triggered by VM")
    return connect(TEST1)

def test2(trigger):
    print(INFO, trigger.rstrip(),  "triggered by VM")
    return connect(TEST2)

def test3(trigger):
    print(INFO, trigger.rstrip(),  "triggered by VM")
    return connect(TEST3)

def test4(trigger):
    print(INFO, trigger.rstrip(),  "triggered by VM")
    connect(TEST4)

def test5(trigger):
    print(INFO, trigger.rstrip(),  "triggered by VM")
    listen(TEST5)


# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event-handler
vm.on_output("TEST1", test1)
vm.on_output("TEST2", test2)
vm.on_output("TEST3", test3)
vm.on_output("TEST4", test4)
vm.on_output("TEST5", test5)

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.cmake().boot(120,image_name='net_tcp').clean()
