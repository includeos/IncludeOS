#!/usr/bin/python
import socket
import sys
import random

def botname():
    return "bot" + str(random.random() * 1000)

class Bot:
  def __init__(self):
      # Create a TCP/IP socket
      self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      self.name = botname()

  def send(self, data):
      self.sock.sendall(data + "\r\n")

  def begin(self):
      # Connect the socket to the port on the server given by the caller
      self.sock.connect(('10.0.0.42', 6667))
      try:
          self.send("NICK bot" + self.name)
          self.send("USER 1 2 3 :444")

          self.send("JOIN #test")

          amount_received = 0
          amount_expected = 10000
          while amount_received < amount_expected:
              self.send("PRIVMSG #test :spamerino cappuchino etc")
              data = self.sock.recv(64)
              amount_received += len(data)
              'print >>sys.stderr, received "%s"' % data

      finally:
          self.sock.close()


bot1 = Bot()
bot1.begin()
