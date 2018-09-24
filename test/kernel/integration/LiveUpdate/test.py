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
    f = open('./service','rb')

    s = socket.socket()
    s.connect(("10.0.0.59", 666))
    s.send(f.read())
    s.close()

vm.on_output("Ready to receive binary blob", begin_test)
vm.cmake().boot(40).clean()
