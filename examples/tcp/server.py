import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('127.0.0.1', 1337)
print >>sys.stderr, 'starting up on %s port %s' % server_address
sock.bind(server_address)

# Listen for incoming connections
sock.listen(5)

while True:
    # Wait for a connection
    print >>sys.stderr, 'waiting for a connection'
    connection, client_address = sock.accept()

    try:
        print >>sys.stderr, 'connection from', client_address

        while True:
            data = connection.recv(1024)
            if data:
                print >>sys.stderr, 'received: %s' % data
                connection.sendall(data)
            else:
                print >>sys.stderr, 'closing', client_address
                break
            
    finally:
        # Clean up the connection
        connection.close()