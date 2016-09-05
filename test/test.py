#!/usr/bin/env python

import subprocess
import sys
import os
import argparse
import json

sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 1) # line buffering
sys.path.insert(0, ".")

from prettify import color as pretty
import validate_test
import validate_all

startdir = os.getcwd()

"""
Script used for running all the valid tests in the terminal.
"""

parser = argparse.ArgumentParser(
  description="IncludeOS testrunner. By default runs all the valid integration tests \
  found in subfolders, the stress test and all unit tests.")

parser.add_argument("-c", "--clean-all", dest="clean", action="store_true",
                    help="Run make clean before building test")

parser.add_argument("-s", "--skip", nargs="*", dest="skip",
                    help="Tests to skip. Valid names: 'unit' (all unit tests), \
                  'stress' (stresstest), 'integration' (all integration tests), examples \
                    or the name of a single integration test folder (leaf node name, e.g. 'udp') ")

parser.add_argument("-t", "--tests", nargs="*", dest="tests",
                    help="Tests to do. Valid names: see '--skip' ")

parser.add_argument("-f", "--fail-early", dest="fail", action="store_true",
                    help="Exit on first failed test")

args = parser.parse_args()

test_count = 0

def print_skipped(name, reason):
  print pretty.WARNING("* Skipping " + name)
  print "  Reason: {0:40}".format(reason)

def valid_tests():
  """Returns a list of all the valid integration tests in */integration/*"""
  if not args.skip:
    args.skip = []

  if "integration" in args.skip:
    return []

  skip_json = json.loads(open("skipped_tests.json").read())
  for skip in skip_json:
    print_skipped(skip["name"], skip["reason"])
    args.skip.append(skip["name"])

  print

  valid_tests = [ x for x in validate_all.valid_tests() if  not x in args.skip ]

  if args.tests:
    return [x for x in valid_tests if x.split("/")[-1] in args.tests]

  return valid_tests


class Test:
  """ A class to start a test as a subprocess and pretty-print status """
  def __init__(self, path, clean = False, command = ['sudo', '-E', 'python', 'test.py'], name = None):

    self.command_ = command
    self.proc_ = None
    self.path_ = path
    self.output_ = None

    if (name == None):
      self.name_ = path
    else:
      self.name_ = name

    print pretty.INFO("Test"), "starting", self.name_

    if clean:
      subprocess.check_output(["make","clean"])
      print pretty.C_GRAY + "\t Cleaned, now start... ", pretty.C_ENDC

  def start(self):
    os.chdir(startdir + "/" + self.path_)
    self.proc_ = subprocess.Popen(self.command_, shell=False,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE)
    os.chdir(startdir)
    return self

  def print_start(self):
    print "* {0:66} ".format(self.name_),
    sys.stdout.flush()

  def wait_status(self):

    self.print_start()

    # Start and wait for the process
    self.output_ = self.proc_.communicate()

    if self.proc_.returncode == 0:
      print pretty.PASS_INLINE()
    else:
      print pretty.FAIL_INLINE()
      print pretty.INFO("Process stdout")
      print pretty.DATA(self.output_[0])
      print pretty.INFO("Process stderr")
      print pretty.DATA(self.output_[1])

    return self.proc_.returncode



def integration_tests():
    """
    Loops over all valid tests as defined by ./validate_all.py. Runs them one by one and gives an update of the statuses at the end.
    """
    global test_count
    valid = valid_tests()
    if not valid:
      print pretty.WARNING("Integration tests skipped")
      return 0

    test_count += len(valid)
    print pretty.HEADER("Starting " + str(len(valid)) + " integration test(s)")
    processes = []

    fail_count = 0
    for path in valid:
        processes.append(Test(path, clean = args.clean).start())

    # Collect test results
    print pretty.HEADER("Collecting integration test results")

    for p in processes:
      fail_count += 1 if p.wait_status() else 0

    # Exit early if any tests failed
    if fail_count and args.fail:
      print pretty.FAIL(str(fail_count) + "integration tests failed")
      sys.exit(fail_count)

    return fail_count


def unit_tests():
  """Perform unit tests"""
  global test_count
  test_count += 1
  if ("unit" in args.skip):
    print pretty.WARNING("Unit tests skipped")
    return 0
  print pretty.HEADER("Building and running unit tests")
  build_status = Test(".", name="Unit test build", command=["make"], clean = args.clean).start().wait_status()
  unit_status = Test(".", name="Unit tests", command = ["./test.lest"]).start().wait_status()

  if (build_status or unit_status) and args.fail:
    print pretty.FAIL("Unit tests failed")
    sys.exit(max(build_status, unit_status))

  return max(build_status, unit_status)

def stress_test():
  """Perform stresstest"""
  global test_count
  test_count += 1
  if ("stress" in args.skip):
    print pretty.WARNING("Stress test skipped")
    return 0

  if (not validate_test.validate_path("stress")):
    raise Exception("Stress test failed validation")

  print pretty.HEADER("Starting stress test")
  stress = Test("stress", clean = args.clean).start()

  if (stress and args.fail):
    print pretty.FAIL("Stress test failed")
    sys.exit(stress)

  return 1 if stress.wait_status() else 0

def examples_working():
  global test_count
  if ("examples" in args.skip):
    print pretty.WARNING("Examples test skipped")
    return 0

  examples_dir = '../examples'
  dirs = os.walk(examples_dir).next()[1]
  print pretty.HEADER("Building " + str(len(dirs)) + " examples")
  test_count += len(dirs)
  fail_count = 0
  for directory in dirs:
    example = examples_dir + "/" + directory
    print "Building Example ", example
    build = Test(example, command = ["make"], name = directory + " build").start().wait_status()
    run = 0 #TODO: Make a 'test' folder for each example, containing test.py, vm.json etc.
    fail_count += 1 if build or run else 0
  return fail_count

def main():
  global test_count

  # Warned about skipped tests
  # @note : doesn't warn if you use '-t a b c ...' to run specific tests
  if args.skip:
    for skip in args.skip:
      print_skipped(skip, "marked skipped on command line")


  test_categories = ["integration", "examples", "unit", "stress"]
  if args.tests:
    test_categories = [x for x in test_categories if x in args.tests or x == "integration"]
  if args.skip:
    test_categories = [x for x in test_categories if not x in args.skip]

  integration = integration_tests() if "integration" in test_categories else 0
  stress = stress_test() if "stress" in test_categories else 0
  unit = unit_tests() if "unit" in test_categories else 0
  examples = examples_working() if "examples" in test_categories else 0

  status = max(integration, stress, unit, examples)

  if (not test_count):
    print "No tests selected"
    exit(0)

  if (status == 0):
    print pretty.SUCCESS(str(test_count - status) + " / " + str(test_count)
                         +  " tests passed, exiting with code 0")
  else:
    print pretty.FAIL(str(status) + " / " + str(test_count) + " tests failed ")

  sys.exit(status)

if  __name__ == '__main__':
    main()
