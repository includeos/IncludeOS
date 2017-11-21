#! /usr/bin/env python
import os
import signal
import sys
import subprocess
import thread

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
import requests

def validateRequest(expected = ""):
    response = requests.get('http://10.0.0.68')
    #print (response.content)
    return (response.content) == expected

# start nodeJS
pro = subprocess.Popen(["node", "server.js"], stdout=subprocess.PIPE)

def startBenchmark(line):
    assert validateRequest("6001")
    assert validateRequest("6002")
    assert validateRequest("6003")
    assert validateRequest("6004")

    assert validateRequest("6001")
    assert validateRequest("6002")
    assert validateRequest("6003")
    assert validateRequest("6004")
    # stop nodeJS
    pro.kill()
    vm._on_success("SUCCESS")
    return True

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event for testing server
vm.on_output("Running [ Micro Load Balancer ]", startBenchmark)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(20).clean()
