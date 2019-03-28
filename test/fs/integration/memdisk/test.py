#! /usr/bin/env python

from builtins import str
import sys
import os

from subprocess import call

from vmrunner import vmrunner
vm = vmrunner.vms[0]

#boot the vm
if len(sys.argv) > 1:
    vm.boot(20,image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(20,image_name='fs_memdisk').clean()
