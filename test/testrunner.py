#!/usr/bin/env python

import subprocess
import sys
import os
import argparse
import json
import time
import multiprocessing  # To figure out number of cpus
import junit_xml as jx
import codecs

sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 1) # line buffering
sys.path.insert(0, ".")
sys.path.insert(0, "..")

from vmrunner.prettify import color as pretty
from vmrunner import validate_vm
import validate_tests

startdir = os.getcwd()

test_categories = ['fs', 'hw', 'kernel', 'mod', 'net', 'performance', 'plugin', 'posix', 'stl', 'util']
test_types = ['integration', 'stress', 'unit', 'misc', 'linux']

"""
Script used for running all the valid tests in the terminal.
"""

parser = argparse.ArgumentParser(
  description="IncludeOS testrunner. By default runs all the valid integration tests \
  found in subfolders, the stress test and all unit tests.")

parser.add_argument("-c", "--clean-all", dest="clean", action="store_true",
                    help="Run make clean before building test")

parser.add_argument("-C", "--clean-only", dest="clean_only", action="store_true",
                    help="Will clean all the test folders and not run tests")

parser.add_argument("-s", "--skip", nargs="*", dest="skip", default=[],
                    help="Tests to skip. Valid names: 'unit' (all unit tests), \
                  'stress' (stresstest), 'integration' (all integration tests), misc \
                    or the name of a single integration test category (e.g. 'fs') ")

parser.add_argument("-t", "--tests", nargs="*", dest="tests", default=[],
                    help="Tests to do. Valid names: see '--skip' ")

parser.add_argument("-f", "--fail-early", dest="fail", action="store_true",
                    help="Exit on first failed test")

parser.add_argument("-j", "--junit-xml", dest="junit", action="store_true",
                    help="Produce junit xml results")

parser.add_argument("-p", "--parallel-tests", dest="parallel", default=0, type=int,
                    help="How many tests to run at once in parallell, \
                    overrides cpu count which is default")

args = parser.parse_args()

test_count = 0

def print_skipped(tests):
    for test in tests:
        if test.skip_:
            print pretty.WARNING("* Skipping " + test.name_)
            print "Reason: {0:40}".format(test.skip_reason_)


class Test:
    """ A class to start a test as a subprocess and pretty-print status """
    def __init__(self, path, clean=False, command=['python', '-u', 'test.py'], name=None):
        self.command_ = command
        self.proc_ = None
        self.path_ = path
        self.output_ = []
        self.clean = clean
        self.start_time = None
        self.properties_ = {"time_sensitive": False, "intrusive": False}
        # Extract category and type from the path variable
        # Category is linked to the top level folder e.g. net, fs, hw
        # Type is linked to the type of test e.g. integration, unit, stress
        if self.path_.split("/")[1] == 'stress':
            self.category_ = 'stress'
            self.type_ = 'stress'
        elif self.path_.split("/")[1] == 'misc':
            self.category_ = 'misc'
            self.type_ = 'misc'
            self.command_ = ['./test.sh']
        elif self.path_.split("/")[1] == 'linux':
            self.category_ = 'linux'
            self.type_ = 'linux'
            self.command_ = ['./test.sh']
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

        # Check for test properties in vm.json
        json_file = os.path.join(self.path_, "vm.json")
        json_file = os.path.abspath(json_file)
        try:
            with open(json_file) as f:
                json_output = json.load(f)
        except IOError:
            json_output = []

        for test_property in self.properties_.keys():
            if test_property in json_output:
                if json_output[test_property]:
                    self.properties_[test_property] = True


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
                'properties: {x[properties_]} \n'
        ).format(x=self.__dict__)

    def start(self):
        self.start_time = time.time()
        os.chdir(startdir + "/" + self.path_)
        if self.clean:
            self.clean_test()

        # execute setup.sh if it exists
        if os.path.isfile('setup.sh'):
            subprocess.call(['./setup.sh'])
        # start test
        logfile_stdout = open('log_stdout.log', 'w')
        logfile_stderr = open('log_stderr.log', 'w')
        self.proc_ = subprocess.Popen(self.command_, shell=False,
                                      stdout=logfile_stdout,
                                      stderr=logfile_stderr)
        os.chdir(startdir)
        return self

    def clean_test(self):
        """ Clean the test directory of all build files"""

        os.chdir(startdir + "/" + self.path_)

        subprocess.check_output(["rm","-rf","build"])
        print pretty.C_GRAY + "\t Cleaned", os.getcwd(), pretty.C_ENDC
        return


    def print_start(self):
        print "* {0:59} ".format(self.name_),
        sys.stdout.flush()

    def print_duration(self):
        print "{0:5.0f}s".format(time.time() - self.start_time),
        sys.stdout.flush()

    def wait_status(self):

        self.print_start()

        # Start and wait for the process
        self.proc_.communicate()
        self.print_duration()

        with codecs.open('{}/log_stdout.log'.format(self.path_), encoding='utf-8', errors='replace') as log_stdout:
            self.output_.append(log_stdout.read())

        with codecs.open('{}/log_stderr.log'.format(self.path_), encoding='utf-8', errors='replace') as log_stderr:
            self.output_.append(log_stderr.read())


        if self.proc_.returncode == 0:
            print pretty.PASS_INLINE()
        else:
            print pretty.FAIL_INLINE()
            print pretty.INFO("Process stdout")
            print pretty.DATA(self.output_[0].encode('ascii', 'ignore').decode('ascii'))
            print pretty.INFO("Process stderr")
            print pretty.DATA(self.output_[1].encode('ascii', 'ignore').decode('ascii'))

        return self.proc_.returncode

    def check_valid(self):
        """ Will check if a test is valid. The following points can declare a test valid:
        1. Contains the files required
        2. Not listed in the skipped_tests.json
        3. Not listed in the args.skip cmd line argument

        Arguments:
        self: Class function
        """
        # If test is misc test, it does not need validation
        if self.type_ == "misc":
            self.skip_ = False
            self.skip_reason_ = None
            return

        # Linux tests only need a test.sh
        if self.type_ == "linux":
            for f in ["CMakeLists.txt", "test.sh"]:
                if not os.path.isfile(self.path_ + "/" + f):
                    self.skip_ = True
                    self.skip_reason_ = 'Missing required file: ' + f
                    return
            self.skip_ = False
            self.skip_reason_ = None
            return

        # Figure out if the test should be skipped
        # Test 1
        if self.path_ in args.skip or self.category_ in args.skip:
            self.skip_ = True
            self.skip_reason_ = 'Defined by cmd line argument'
            return

        # Test 2
        valid, err = validate_tests.validate_test(self.path_, verb = False)
        if not valid:
            self.skip_ = True
            self.skip_reason_ = err
            return

        # Test 3
        skip_json = json.loads(open("skipped_tests.json").read())
        for skip in skip_json:
            if skip['name'] in self.path_:
                self.skip_ = True
                self.skip_reason_ = skip['reason']
                return

        self.skip_ = False
        self.skip_reason_ = None
        return


def stress_test(stress_tests):
    """Perform stresstest"""
    global test_count
    test_count += len(stress_tests)
    if len(stress_tests) == 0:
        return 0

    if ("stress" in args.skip):
        print pretty.WARNING("Stress test skipped")
        return 0

    if (not validate_tests.validate_test("stress")):
        raise Exception("Stress test failed validation")

    print pretty.HEADER("Starting stress test")
    for test in stress_tests:
        test.start()

    for test in stress_tests:
        return 1 if test.wait_status() else 0


def misc_working(misc_tests, test_type):
    global test_count
    test_count += len(misc_tests)
    if len(misc_tests) == 0:
        return 0

    if ("misc" in args.skip):
        print pretty.WARNING("Misc test skipped")
        return 0

    print pretty.HEADER("Building " + str(len(misc_tests)) + " " + str(test_type))
    fail_count = 0

    for test in misc_tests:
        build = test.start().wait_status()
        fail_count += 1 if build else 0

    return fail_count


def integration_tests(tests):
    """ Function that runs the tests that are passed to it.
    Filters out any invalid tests before running

    Arguments:
        tests: List containing test objects to be run

    Returns:
        integer: Number of tests that failed
    """
    if len(tests) == 0:
        return 0

    time_sensitive_tests = [ x for x in tests if x.properties_["time_sensitive"] ]
    tests = [ x for x in tests if x not in time_sensitive_tests ]

    # Print info before starting to run
    print pretty.HEADER("Starting " + str(len(tests)) + " integration test(s)")
    for test in tests:
        print pretty.INFO("Test"), "starting", test.name_

    if time_sensitive_tests:
        print pretty.HEADER("Then starting " + str(len(time_sensitive_tests)) + " time sensitive integration test(s)")
        for test in time_sensitive_tests:
            print pretty.INFO("Test"), "starting", test.name_

    processes = []
    fail_count = 0
    global test_count
    test_count += len(tests) + len(time_sensitive_tests)

    # Find number of cpu cores
    if args.parallel:
        num_cpus = args.parallel
    else:
        num_cpus = multiprocessing.cpu_count()
    num_cpus = int(num_cpus)

	# Collect test results
    print pretty.HEADER("Collecting integration test results, on {0} cpu(s)".format(num_cpus))

    # Run a maximum number of parallell tests equal to cpus available
    while tests or processes:   # While there are tests or processes left
        try:
            processes.append(tests.pop(0).start())  # Remove test from list after start
        except IndexError:
            pass   # All tests done

        while (len(processes) == num_cpus) or not tests:
            # While there are a maximum of num_cpus to process
            # Or there are no more tests left to start we wait for them
            for p in list(processes):   # Iterate over copy of list
                if p.proc_.poll() is not None:
                    fail_count += 1 if p.wait_status() else 0
                    processes.remove(p)

            time.sleep(1)
            if not processes and not tests:
                break

        # Exit early if any tests failed
        if fail_count and args.fail:
            print pretty.FAIL(str(fail_count) + "integration tests failed")
            sys.exit(fail_count)

    # Start running the time sensitive tests
    for test in time_sensitive_tests:
        process = test.start()
        fail_count += 1 if process.wait_status() else 0
        if fail_count and args.fail:
            print pretty.FAIL(str(fail_count) + "integration tests failed")
            sys.exit(fail_count)

    return fail_count


def find_test_folders():
    """ Used to find all (integration) test folders

    Returns:
        List: list of string with path to all leaf nodes
    """
    leaf_nodes = []

    root = "."

    for directory in os.listdir(root):
        path = [root, directory]

        # Only look in folders listed as a test category
        if directory in test_types:
            if directory == 'misc' or directory == 'linux':
                # For each subfolder in misc, register test
                for subdir in os.listdir("/".join(path)):
                    path.append(subdir)
                    leaf_nodes.append("/".join(path))
                    path.pop()
            elif directory == 'stress':
                leaf_nodes.append("./stress")
        elif directory not in test_categories:
            continue

        # Only look into subfolders named "integration"
        for subdir in os.listdir("/".join(path)):
            if subdir != "integration":
                continue

            # For each subfolder in integration, register test
            path.append(subdir)
            for testdir in os.listdir("/".join(path)):
                path.append(testdir)

                if (not os.path.isdir("/".join(path))):
                    continue

                # Add test directory
                leaf_nodes.append("/".join(path))
                path.pop()
            path.pop()

    return leaf_nodes


def filter_tests(all_tests, arguments):
    """ Will figure out which tests are to be run

    Arguments:
        all_tests (list of Test obj): all processed test objects
        arguments (argument object): Contains arguments from argparse

    returns:
        tuple: (All Test objects that are to be run, skipped_tests)
    """
    print pretty.HEADER("Filtering tests")

    # Strip trailing slashes from paths
    add_args = [ x.rstrip("/") for x in arguments.tests ]
    skip_args = [ x.rstrip("/") for x in arguments.skip ]

    print pretty.INFO("Tests to run"), ", ".join(add_args)

    # 1) Add tests to be run

    # If no tests specified all are run
    if not add_args:
        tests_added = [ x for x in all_tests if x.type_ in test_types ]
    else:
        tests_added = [ x for x in all_tests
                        if x.type_ in add_args
                        or x.category_ in add_args
                        or x.name_ in add_args]

        # Deal with specific properties
        add_properties = list(set(add_args).intersection(all_tests[0].properties_.keys()))
        for test in all_tests:
            for argument in add_properties:
                if test.properties_[argument] and test not in tests_added:
                    tests_added.append(test)




    # 2) Remove tests defined by the skip argument
    print pretty.INFO("Tests marked skip on command line"), ", ".join(skip_args)
    skipped_tests = [ x for x in tests_added
                  if x.type_ in skip_args
                  or x.category_ in skip_args
                  or x.name_ in skip_args
                  or x.skip_]

    # Deal with specific properties
    skip_properties = list(set(skip_args).intersection(all_tests[0].properties_.keys()))
    for test in tests_added:
        for argument in skip_properties:
            if test.properties_[argument] and test not in skipped_tests:
                test.skip_ = True
                test.skip_reason_ = "Test marked skip on command line"
                skipped_tests.append(test)

    # Print all the skipped tests
    print_skipped(skipped_tests)

    fin_tests = [ x for x in tests_added if x not in skipped_tests ]
    print pretty.INFO("Accepted tests"), ", ".join([x.name_ for x in fin_tests])

    return (fin_tests, skipped_tests)

def create_junit_output(tests):
    """ Creates an output file for generating a junit test report

    args:
        tests: All tests completed + skipped

    returns:
        boolean: False if file generation failed - NOT YET
    """

    # Populate junit objects for all performed tests
    junit_tests = []

    # Integration tests
    for test in tests:
        junit_object = jx.TestCase(test.name_, classname="IncludeOS.{}.{}".format(test.type_, test.category_))

        # If test is skipped add skipped info
        if test.skip_:
            junit_object.add_skipped_info(message = test.skip_reason_, output = test.skip_reason_)
        elif test.proc_.returncode is not 0:
            junit_object.add_failure_info(output = test.output_[0])
        else:
            junit_object.stdout = test.output_[0]
            junit_object.stderr = test.output_[1]

        # Add to list of all test objects
        junit_tests.append(junit_object)

    # Stress and misc tests
    ts = jx.TestSuite("IncludeOS tests", junit_tests)
    with open('output.xml', 'w') as f:
            jx.TestSuite.to_file(f, [ts], prettyprint=False)


def main():
    # Find leaf nodes
    leaves = find_test_folders()

    # Populate test objects
    all_tests = [ Test(path, args.clean) for path in leaves ]

    # Check if clean-only is issued
    if args.clean_only:
        for test in all_tests:
            test.clean_test()
        sys.exit(0)

    # get a list of all the tests that are to be run
    filtered_tests, skipped_tests = filter_tests(all_tests, args)

    # Run the tests
    integration_result = integration_tests([x for x in filtered_tests if x.type_ == "integration"])
    stress_result = stress_test([x for x in filtered_tests if x.type_ == "stress"])
    misc_result = misc_working([x for x in filtered_tests if x.type_ == "misc"], "misc")
    linux_result = misc_working([x for x in filtered_tests if x.type_ == "linux"], "linux platform")

    # Print status from test run
    status = max(integration_result, stress_result, misc_result, linux_result)
    if (status == 0):
        print pretty.SUCCESS(str(test_count - status) + " / " + str(test_count)
                            +  " tests passed, exiting with code 0")
    else:
        print pretty.FAIL(str(status) + " / " + str(test_count) + " tests failed ")

    # Create Junit output
    if args.junit:
        create_junit_output(filtered_tests + skipped_tests)

    sys.exit(status)


if  __name__ == '__main__':
    main()
