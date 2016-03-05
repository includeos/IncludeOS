import socket
import sys

def connect(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('127.0.0.1', port)
    print >>sys.stderr, 'connecting to %s port %s' % server_address
    sock.connect(server_address)

    try:
        while True:
            data = sock.recv(1024)
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