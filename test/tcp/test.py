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
connect(8084)

def listen(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('127.0.0.1', 1337)
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

listen(1337)