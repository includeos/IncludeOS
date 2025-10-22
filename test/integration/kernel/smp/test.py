#!/usr/bin/env python3

from builtins import str
import sys
import os

from vmrunner import vmrunner

boot_count = 0
success = False

def booted(line):
    global boot_count
    boot_count += 1;
    if boot_count > 1:
        vm.exit(1, "VM rebooted unexpectedly")

vm = vmrunner.vms[0]

vm.on_output("#include<os> // Literally", booted);

vm.boot(20,image_name='kernel_smp.elf.bin')
