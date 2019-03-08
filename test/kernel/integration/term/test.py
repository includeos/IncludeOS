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
    s = socket.socket()
    s.connect(("10.0.0.63", 23))
    s.send("netstat\r\n")
    result = s.recv(1024)
    print result
    s.close()
    return "Banana Terminal" in result

vm.on_output("Connect to terminal", begin_test)
if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(40,image_name='kernel_term').clean()
