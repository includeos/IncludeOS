#! /usr/bin/env python
import os
import signal
import sys
import subprocess
import thread
import time
import atexit

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
import requests
expected_string = "#" * 1024 * 50

def validateRequest(addr):
    response = requests.get('https://10.0.0.68:443', verify=False)
    #print (response.content)
    return (response.content) == str(addr) + expected_string

# start nodeJS
pro = subprocess.Popen(["nodejs", "server.js"], stdout=subprocess.PIPE)

requests_completed = False
def startBenchmark(line):
    print "<test.py> starting test "
    assert validateRequest(6001)
    assert validateRequest(6002)
    assert validateRequest(6003)
    assert validateRequest(6004)

    assert validateRequest(6001)
    assert validateRequest(6002)
    assert validateRequest(6003)
    assert validateRequest(6004)
    print "Waiting for TCP MSL end..."
    global requests_completed
    requests_completed = True
    return True
def mslEnded(line):
    if requests_completed:
        vm._on_success("SUCCESS")
    return True

@atexit.register
def cleanup():
    print "<test.py> Stopping node server"
    # stop nodeJS
    pro.kill()


# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event for testing server
vm.on_output("MicroLB ready for test", startBenchmark)
vm.on_output("TCP MSL ended", mslEnded)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(60).clean()
