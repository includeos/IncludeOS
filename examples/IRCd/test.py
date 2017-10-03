#!/usr/bin/python
import socket
import sys
import random
from inspect import getmodule
from multiprocessing import Pool


def async(decorated):
    r'''Wraps a top-level function around an asynchronous dispatcher.

        when the decorated function is called, a task is submitted to a
        process pool, and a future object is returned, providing access to an
        eventual return value.

        The future object has a blocking get() method to access the task
        result: it will return immediately if the job is already done, or block
        until it completes.

        This decorator won't work on methods, due to limitations in Python's
        pickling machinery (in principle methods could be made pickleable, but
        good luck on that).
    '''
    # Keeps the original function visible from the module global namespace,
    # under a name consistent to its __name__ attribute. This is necessary for
    # the multiprocessing pickling machinery to work properly.
    module = getmodule(decorated)
    decorated.__name__ += '_original'
    setattr(module, decorated.__name__, decorated)

    def send(*args, **opts):
        return async.pool.apply_async(decorated, args, opts)

    return send

def botname():
    return "bot" + str(int(random.random() * 10000))

class Bot:
  def __init__(self):
      # Create a TCP/IP socket
      self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
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
              self.send("NICK " + self.name)
              self.send("USER 1 2 3 :444")

              self.send("JOIN #test")

              amount_received = 0
              amount_expected = 10
              while amount_received < amount_expected:
                  data = self.sock.recv(64)
                  amount_received += len(data)
                  'print >>sys.stderr, received "%s"' % data

              self.send("PRIVMSG #test :spamerino cappuchino etc")
              self.send("QUIT :Lates")
          finally:
              self.sock.close()
      finally:
          return


if __name__ == '__main__':
  while True:
      bot = Bot()
      try:
          bot.begin()
      finally:
          ''
