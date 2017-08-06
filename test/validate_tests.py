#!/usr/bin/env python
import sys
sys.path.insert(0, ".")
sys.path.insert(0, "..")

import subprocess
import os
from vmrunner import validate_vm
from vmrunner.prettify import color

# Verify if a given path is a valid integration test
def validate_test(path, verb = False):

    try:
        # Load any valid configs in path
        valid_configs = validate_vm.load_config(path)
        if not valid_configs:
            raise Exception("No valid JSON config in path")

        # Additional requirements for tests
        required_files = ["CMakeLists.txt", "test.py"]

        for f in required_files:
            if not os.path.isfile(path + "/" + f):
                raise Exception("Required file " + f + " missing")

        if verb:
            print "<validate_test> \tPASS: ",path
        return True, "PASS"

    except Exception as err:
        if verb:
            print "<validate_test> \tFAIL: ", path, ": " , err.message
        
        return False, err.message


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
          passed, err = validate_test(path, verb)
          if passed:
            tests.append(path)

  return tests

if __name__ == "__main__":
  valid_tests(verb = True)
