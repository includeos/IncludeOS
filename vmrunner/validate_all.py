#!/usr/bin/env python
import sys
sys.path.insert(0, ".")

import subprocess
import os
import validate_test

from prettify import color

def valid_tests(verb = False):
  tests = []

  dirs = os.walk('.').next()[1]
  for directory in  dirs:
    subdirs = os.walk(directory).next()[1]
    if "integration" in subdirs:
      subdirs = os.walk(directory + "/integration").next()[1]
      if subdirs:
        for d in subdirs:
          path = directory + "/integration/" + d
          if validate_test.validate_path(path, verb):
            tests.append(path)
          else:
            print color.WARNING("Validator: " + path + " failed validation")

  return tests

if __name__ == "__main__":
  valid_tests(verb = True)
