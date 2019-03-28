#! /usr/bin/env python

from builtins import str
import sys
import os

from vmrunner import vmrunner
vm=vmrunner.vms[0]


if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    #the corutines is set in the CMakelists.
    vm.cmake().boot(40,image_name='stl_crt').clean()
