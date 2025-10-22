#!/usr/bin/env python3

from builtins import str
import sys
import os

from vmrunner import vmrunner
vm = vmrunner.vms[0]

vm.boot(20,image_name='posix_pthread.elf.bin')
