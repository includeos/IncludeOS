#! /usr/bin/env python
from __future__ import print_function
from builtins import str
import sys
import os
import socket

from vmrunner import vmrunner
vm = vmrunner.vms[0]


def begin_test(line):
    s = socket.socket()
    s.connect(("10.0.0.63", 23))
    s.send(str.encode("netstat\r\n"))
    result = s.recv(1024).decode()
    print(result)
    s.close()
    return "Banana Terminal" in result

vm.on_output("Connect to terminal", begin_test)
if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(40,image_name='kernel_term').clean()
