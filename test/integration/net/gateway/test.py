#!/usr/bin/env python3

from builtins import str
import sys
import os

from vmrunner import vmrunner

#if name is passed execute that do not clean and do not rebuild..
if len(sys.argv) > 1:
    vmrunner.vms[0].boot(image_name=str(sys.argv[1]))
else:
    vmrunner.vms[0].boot(70,image_name='net_gateway.elf.bin')
