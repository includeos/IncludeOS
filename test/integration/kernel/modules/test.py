#!/usr/bin/env python3

import sys
import os

from vmrunner import vmrunner

vm = vmrunner.vms[0];
# Build, run and clean
vm.boot(image_name='kernel_modules.elf.bin')
