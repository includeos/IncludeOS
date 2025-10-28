#!/usr/bin/env python3

from builtins import str
import sys
import os

from vmrunner import vmrunner
#TODO move timeout to ctest..
if len(sys.argv) > 1:
    vmrunner.vms[0].boot(image_name=str(sys.argv[1]))
else:
    vmrunner.vms[0].boot(30,image_name='net_configure.elf.bin')
