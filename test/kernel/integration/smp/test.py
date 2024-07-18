#!/usr/bin/env python3

from builtins import str
import sys
import os

from vmrunner import vmrunner

vmrunner.vms[0].boot(20,image_name='kernel_smp.elf.bin')
#vm.cmake(["-Dsingle_threaded=OFF"]).boot(20).clean()
