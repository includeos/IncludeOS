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

test_categories = ['fs', 'hw', 'kernel', 'mod', 'net', 'performance', 'platform', 'posix', 'stl', 'util']
test_types = ['integration', 'stress', 'unit', 'misc']

"""
Script used for running all the valid tests in the terminal.
"""

parser = argparse.ArgumentParser(
  description="IncludeOS testrunner. By default runs all the valid integration tests \
  found in subfolders, the stress test and all unit tests.")

parser.add_argument("-c", "--clean-all", dest="clean", action="store_true",
                    help="Run make clean before building test")

parser.add_argument("-s", "--skip", nargs="*", dest="skip", default=[],
                    help="Tests to skip. Valid names: 'unit' (all unit tests), \
                  'stress' (stresstest), 'integration' (all integration tests), misc \
                    or the name of a single integration test folder (leaf node name, e.g. 'udp') ")

parser.add_argument("-t", "--tests", nargs="*", dest="tests", default=[],
                    help="Tests to do. Valid names: see '--skip' ")

parser.add_argument("-f", "--fail-early", dest="fail", action="store_true",
                    help="Exit on first failed test")

args = parser.parse_args()

test_count = 0

def print_skipped(tests):
    for test in tests:
        if test.skip_:
            print pretty.WARNING("* Skipping " + test.name_)
            print "  Reason: {0:40}".format(test.skip_reason_)


class Test:
  """ A class to start a test as a subprocess and pretty-print status """
  def __init__(self, path, clean=False, command=['sudo', '-E', 'python', 'test.py'], name=None):

    self.command_ = command
    self.proc_ = None
    self.path_ = path
    self.output_ = None

    # Extract category and type from the path variable
    # Category is linked to the top level folder e.g. net, fs, hw
    # Type is linked to the type of test e.g. integration, unit, stress
    if self.path_ == 'stress':
      self.category_ = 'stress'
      self.type_ = 'stress'
    elif self.path_.split("/")[0] == 'misc':
      self.category_ = 'misc'
      self.type_ = 'misc'
    elif self.path_ == 'mod/gsl':
      self.category_ = 'mod'
      self.type_ = 'mod'
    elif self.path_ == '.':
      self.category_ = 'unit'
      self.type_ = 'unit'
    else:
      self.category_ = self.path_.split('/')[-3]
      self.type_ = self.path_.split('/')[-2]

    if not name:
      self.name_ = path
    else:
      self.name_ = name

    # Check if the test is valid or not
    self.check_valid()

    if clean:
      subprocess.check_output(["make","clean"])
      print pretty.C_GRAY + "\t Cleaned, now start... ", pretty.C_ENDC

  def __str__(self):
      """ Print output about the test object """

      return ('name_: {x[name_]} \n'
              'path_: {x[path_]} \n'
              'command_: {x[command_]} \n'
              'proc_: {x[proc_]} \n'
              'output_: {x[output_]} \n'
              'category_: {x[category_]} \n'
              'type_: {x[type_]} \n'
              'skip: {x[skip_]} \n'
              'skip_reason: {x[skip_reason_]} \n'
              ).format(x=self.__dict__)

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

  def check_valid(self):
    """ Will check if a test is valid. The following points can declare a test valid:
        1. Contains the files required
        2. Not listed in the skipped_tests.json
        3. Not listed in the args.skip cmd line argument

    Arguments:
        self: Class function
    """
    # Test 1
    if not validate_test.validate_path(self.path_, verb = False):
        self.skip_ = True
        self.skip_reason_ = 'Failed validate_test, missing files'
        return

    # Test 2
    # Figure out if the test should be skipped
    skip_json = json.loads(open("skipped_tests.json").read())
    for skip in skip_json:
        if self.path_ == skip['name']:
            self.skip_ = True
            self.skip_reason_ = skip['reason']
            return

    # Test 3
    if self.path_ in args.skip or self.category_ in args.skip:
            self.skip_ = True
            self.skip_reason_ = 'Defined by cmd line argument'
            return

    self.skip_ = False
    self.skip_reason_ = None
    return


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


def misc_working():
  global test_count
  if ("misc" in args.skip):
    print pretty.WARNING("Misc test skipped")
    return 0

  misc_dir = 'misc'
  dirs = os.walk(misc_dir).next()[1]
  dirs.sort()
  print pretty.HEADER("Building " + str(len(dirs)) + " misc")
  test_count += len(dirs)
  fail_count = 0
  for directory in dirs:
    misc = misc_dir + "/" + directory
    print "Building misc ", misc
    build = Test(misc, command = ['./test.sh'], name = directory).start().wait_status()
    run = 0 #TODO: Make a 'test' folder for each miscellanous test, containing test.py, vm.json etc.
    fail_count += 1 if build or run else 0
  return fail_count


def integration_tests(tests):
    """ Function that runs the tests that are passed to it.
    Filters out any invalid tests before running

    Arguments:
        tests: List containing test objects to be run

    Returns:
        integer: Number of tests that failed
    """

    # Only run the valid tests
    tests = [ x for x in tests if not x.skip_ and x.type_ == 'integration' ]

    # Print info before starting to run
    print pretty.HEADER("Starting " + str(len(tests)) + " integration test(s)")
    for test in tests:
        print pretty.INFO("Test"), "starting", test.name_

    processes = []
    fail_count = 0
    global test_count
    test_count += len(tests)

    # Start running tests in parallell
    for test in tests:
        processes.append(test.start())

	# Collect test results
    print pretty.HEADER("Collecting integration test results")

    for p in processes:
        fail_count += 1 if p.wait_status() else 0

    # Exit early if any tests failed
    if fail_count and args.fail:
        print pretty.FAIL(str(fail_count) + "integration tests failed")
        sys.exit(fail_count)

    return fail_count


def find_leaf_nodes():
    """ Used to find all leaf nodes in the test directory,
    this is to help identify all possible test directories.
    Only looks in folders that actually store tests

    Returns:
        List: list of string with path to all leaf nodes
    """
    leaf_nodes = []

    for dirpath, dirnames, filenames in os.walk('.'):
        # Will now skip any path that is not defined as a test category
        # or ends with unit or integration -> no tests in those folders were
        # created
        if dirpath[2:].split('/')[0] in test_categories and dirpath.split('/')[-1] not in ['unit', 'integration']:
            if len(dirpath[2:].split('/')) <= 3 and not dirnames:
                leaf_nodes.append(dirpath[2:])

    return leaf_nodes


def main():
    # Find leaf nodes
    leaves = find_leaf_nodes()

    # Populate test objects
    all_tests = [ Test(path) for path in leaves ]

    # Figure out which tests are to be run
    test_categories_to_run = []
    test_types_to_run = []
    if args.tests:
        for argument in args.tests:
            if argument in test_categories and argument not in args.skip:
                test_categories_to_run.append(argument)
            elif argument in test_types and argument not in args.skip:
                test_types_to_run.append(argument)
            else:
                print 'Test specified is not recognised, exiting'
                sys.exit(1)
    else:
        test_types_to_run = test_types


    if test_categories_to_run:
        # This means that a specific category has been requested
        specific_tests = [ test for test in all_tests if test.category_ in test_categories_to_run ]

        # Print which tests are skipped
        print_skipped(specific_tests)

        # Run the tests
        integration = integration_tests(specific_tests)
    else:
        # Print which tests are skipped
        print_skipped(all_tests)

        # Run the tests
        integration = integration_tests(all_tests) if "integration" in test_types_to_run else 0

    stress = stress_test() if "stress" in test_types_to_run else 0
    unit = unit_tests() if "unit" in test_types_to_run else 0
    misc = misc_working() if "misc" in test_types_to_run else 0

    status = max(integration, stress, unit, misc)
    if (status == 0):
        print pretty.SUCCESS(str(test_count - status) + " / " + str(test_count)
                            +  " tests passed, exiting with code 0")
    else:
        print pretty.FAIL(str(status) + " / " + str(test_count) + " tests failed ")

    sys.exit(status)


if  __name__ == '__main__':
    main()
