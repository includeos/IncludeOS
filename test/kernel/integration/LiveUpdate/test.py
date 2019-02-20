#! /usr/bin/env python
import sys
import os
import socket

includeos_src = os.environ.get('INCLUDEOS_SRC',
           os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

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
