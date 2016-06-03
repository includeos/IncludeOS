#!/usr/bin/env python

import subprocess
import sys
import time
import os

sys.path.insert(0,".")

"""
Script used for running all the valid tests in the terminal. 
"""

def valid_tests():
    """ Returns a list of all the valid tests in the test folder 

    returns: list"""
    valid_tests = subprocess.check_output(['./validate_all.sh', '-onlyNames'])
    return valid_tests.splitlines()

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

    print ">>> This script runs all the valid tests in the tests folder. These are given by the ./validate_all.sh script"
    print ">>> Will perform the following tests:"
    for test in valid_tests():
        print test, 
    print "/n"
    
    result_list = []
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
        
        result_list.append((test, test_result))
        os.chdir("..")

    print_result(result_list)

if  __name__ == '__main__':
    main()
