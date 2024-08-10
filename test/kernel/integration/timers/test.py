#!/usr/bin/env python3

from builtins import str
import sys
import os

from vmrunner import vmrunner

vmrunner.vms[0].boot(60, image_name="kernel_timers.elf.bin")
