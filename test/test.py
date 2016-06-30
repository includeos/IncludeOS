#!/usr/bin/env python

import subprocess
import sys
import os
import argparse

sys.path.insert(0, ".")

"""
Script used for running all the valid tests in the terminal.
"""

parser = argparse.ArgumentParser(description="Runs all the tests that are \
                                  ready in the test folder")
parser.add_argument("-s", "--skip_test", nargs="*", dest="skip",
                    help="Tests to skip")

args = parser.parse_args()


def valid_tests():
    """ Returns a list of all the valid tests in the test folder

    returns: list"""
    valid_tests = subprocess.check_output(['./validate_all.sh', '-onlyNames'])
    valid_tests = valid_tests.splitlines()
    if args.skip is not None:
        for test_to_skip in args.skip:
            if test_to_skip in valid_tests:
                valid_tests.remove(test_to_skip)
    return valid_tests


def print_result(result_list):
    """ Used for printing the result of the tests performed """

    for name, result in result_list:
        if result == "PASS":
            color = "\033[42;30m"       # Black text (30) on Green background (42)
        elif result == "FAIL":
            color = "\033[37;41m"       # White text (37) on Red background (41)

        background_color_end = "\033[0m"    # Used to reset the color back to default

        print '{0:15} ==> {3} {1} {2}'.format(name, result, background_color_end, color)


def main():
    """
    Loops over all valid tests as defined by the ./validate_all.sh script. Runs them one by one and gives an update of the statuses at the end.
    """

    print ">>> This script runs all the valid tests in the tests folder."
    print ">>> These are found by the ./validate_all.sh script"
    print ">>> To skip certain tests use -s <tests to skip>\n\n"
    print ">>> Will perform the following tests:"
    for test in valid_tests():
        print test,
    print "\n"

    result_list = []
    all_tests_pass = True
    for test in valid_tests():
        print ">> Now testing {0}".format(test)

        # Change into directory
        os.chdir(test)

        # Perform test.py
        process = subprocess.Popen(['python', 'test.py'], shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        process.wait()

        # Check the output from the test
        if process.returncode == 0:
            test_result = "PASS"
        else:
            test_result = "FAIL"
            print "one test failed, setting False"
            all_tests_pass = False

        result_list.append((test, test_result))
        os.chdir("..")

    print_result(result_list)
    print all_tests_pass
    if not all_tests_pass:
        print 'exiting with 1'
        sys.exit(1)
    else:
        print 'exiting with 0'
        sys.exit(0)


if  __name__ == '__main__':
    main()
