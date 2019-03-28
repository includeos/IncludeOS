#! /usr/bin/env python

from __future__ import division
from __future__ import print_function
from builtins import str
from past.utils import old_div
import sys
import os

from vmrunner import vmrunner

vm = vmrunner.vms[0];

def check_hex(line):
  my_hex = line.split(":")[1].strip()
  res = int(my_hex, 16)
  return res == 100;

format_string_size = None

def set_format_string_size(line):
  global format_string_size

  print("Received format string: ", line)
  format_string_size = int(line.split(":")[1].strip())

def check_truncation(line):
  assert(format_string_size)

  print("Received truncated string: ", line, "of size", len(line), "(format size * ", old_div(len(line),format_string_size),")")
  assert(len(line) <= format_string_size * 2)
  # truncated outputs are unacceptable :)
  assert(line.strip().split(" ")[-1] == "END")

vm.on_output("I can print hex", check_hex)
vm.on_output("String", set_format_string_size)
vm.on_output("truncate", check_truncation)

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Build, run and clean
    vm.cmake().boot(image_name='kernel_kprint').clean()
