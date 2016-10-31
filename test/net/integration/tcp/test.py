#!/usr/bin/python

import socket
import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src + "/test")

import vmrunner

# Usage: python test.py $GUEST_IP $HOST_IP
GUEST = '10.0.0.44' if (len(sys.argv) < 2) else sys.argv[1]
HOST = '10.0.0.1' if (len(sys.argv) < 3) else sys.argv[2]

def TCP_test(trigger_line):
    def connect(port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (GUEST, port)
        print >>sys.stderr, 'connecting to %s port %s' % server_address
        sock.connect(server_address)

        try:
            while True:
                data = sock.recv(1024)
                #print >>sys.stderr, '%s' % data
                if data:
                    sock.sendall(data);
                else:
                    break
        finally:
            print >>sys.stderr, 'closing socket'
            sock.close()
        return

    connect(8081)
    connect(8082)
    connect(8083)
    connect(8084)

    def listen(port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_address = (HOST, port)
        print >>sys.stderr, 'starting up on %s port %s' % server_address
        sock.bind(server_address)
        sock.listen(1)

        while True:
            connection, client_address = sock.accept()
            try:
                print >>sys.stderr, 'connection from', client_address
                while True:
                    data = connection.recv(1024)
                    if data:
                        print >>sys.stderr, 'received data, sending data back to the client'
                        connection.sendall(data)
                        print >>sys.stderr, 'close connection to client'
                        connection.close()
                    else:
                        print >>sys.stderr, 'no more data from', client_address
                        break

            finally:
                connection.close()
                break
            sock.close()
        return

    listen(8085)

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event-handler
vm.on_output("IncludeOS TCP test", TCP_test)

# Boot the VM, taking a timeout as parameter
vm.make().boot(80)
