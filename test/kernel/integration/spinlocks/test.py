#!/usr/bin/env python3

import sys
import os

from vmrunner import vmrunner

vm = vmrunner.vms[0];

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Build, run and clean
    vm.boot(image_name='kernel_spinlocks')
