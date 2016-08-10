#!/usr/bin/python
import socket
import sys
import random

def botname():
    return "bot" + str(int(random.random() * 1000))

class Bot:
  def __init__(self):
      # Create a TCP/IP socket
      self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      self.name = botname()

  def send(self, data):
      try:
          self.sock.sendall(data + "\r\n")
      finally:
          return

  def begin(self):
      # Connect the socket to the port on the server given by the caller
      try:
          self.sock.connect(('10.0.0.42', 6667))
          try:
              self.send("NICK bot" + self.name)
              self.send("USER 1 2 3 :444")

              self.send("JOIN #test")

              amount_received = 0
              amount_expected = 10
              while amount_received < amount_expected:
                  self.send("PRIVMSG #test :spamerino cappuchino etc")
                  data = self.sock.recv(64)
                  amount_received += len(data)
                  'print >>sys.stderr, received "%s"' % data

              self.send("QUIT :Lates")
          finally:
              self.sock.close()
      finally:
          return


while True:
  bot = Bot()
  try:
      bot.begin()
  finally:
      ''
