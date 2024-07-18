#!/usr/bin/env python3
from builtins import str
import sys
import os

from vmrunner import vmrunner
vm=vmrunner.vms[0]

vm.boot(40,image_name='stl_coroutines.elf.bin')
