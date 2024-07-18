#!/usr/bin/env python3

from builtins import str
import sys
import os
import subprocess

from vmrunner import vmrunner

vm = vmrunner.vms[0];

# Use grubify-script
grubify = "grubiso.sh"

# Create the GRUB image
subprocess.check_call(["bash",grubify,"kernel_grub.elf.bin"])
vm.boot(multiboot = False)
