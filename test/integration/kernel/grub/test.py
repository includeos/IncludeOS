#!/usr/bin/env python3

from builtins import str
import sys
import os
import subprocess

from vmrunner import vmrunner

vm = vmrunner.vms[0];

# Create the GRUB image
vm.boot(20, multiboot = False)
