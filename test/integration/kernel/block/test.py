#!/usr/bin/env python3
from builtins import str
import sys
import os


from subprocess import call

from vmrunner import vmrunner
if len(sys.argv) > 1:
    vmrunner.vms[0].boot(image_name=str(sys.argv[1]))
else:
    vmrunner.vms[0].boot(40,image_name='kernel_block.elf.bin')
