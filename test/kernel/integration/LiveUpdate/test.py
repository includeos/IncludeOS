#! /usr/bin/env python
from builtins import str
import sys
import os
import socket

from vmrunner import vmrunner
vm = vmrunner.vms[0]


def begin_test(line):
    f = open('./kernel_LiveUpdate','rb')

    s = socket.socket()
    s.connect(("10.0.0.59", 666))
    s.send(f.read())
    s.close()

vm.on_output("Ready to receive binary blob", begin_test)
if len(sys.argv) > 1:
    vm.boot(40,image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(40,image_name='kernel_LiveUpdate').clean()
