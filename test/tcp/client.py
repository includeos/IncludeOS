import socket
import sys

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('127.0.0.1', 8081)
print >>sys.stderr, 'connecting to %s port %s' % server_address
sock.connect(server_address)

try:
    response = ""
    while True:
        data = sock.recv(1024)
        if data:
            response += data
        else:
            sock.sendall(response)
            break
finally:
    print >>sys.stderr, 'closing socket'
    sock.close()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('127.0.0.1', 8082)
print >>sys.stderr, 'connecting to %s port %s' % server_address
sock.connect(server_address)

try:
    response = ""
    while True:
        data = sock.recv(1024)
        if data:
            response += data
        else:
            sock.sendall(response)
            break
finally:
    print >>sys.stderr, 'closing socket'
    sock.close()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('127.0.0.1', 8083)
print >>sys.stderr, 'connecting to %s port %s' % server_address
sock.connect(server_address)

try:
    response = ""
    while True:
        data = sock.recv(1024)
        if data:
            response += data
        else:
            sock.sendall(response)
            break
finally:
    print >>sys.stderr, 'closing socket'
    sock.close()