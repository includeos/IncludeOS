import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('10.0.0.1', 1337)
print 'starting up on %s port %s' % server_address
sock.bind(server_address)

# Listen for incoming connections
sock.listen(5)

while True:
    # Wait for a connection
    print 'waiting for a connection'
    connection, client_address = sock.accept()

    try:
        print 'connection from', client_address
        bytes = 0

        while True:
            data = connection.recv(8192)
            if data:
                bytes += len(data)
                #print 'received: %d' % len(data)
                connection.sendall(data)
            else:
                print 'received %d bytes' % bytes
                print 'closing', client_address
                break

    finally:
        # Clean up the connection
        connection.close()
