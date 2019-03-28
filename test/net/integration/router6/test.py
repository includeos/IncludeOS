#! /usr/bin/env python

from __future__ import print_function
from future import standard_library
standard_library.install_aliases()
from builtins import str
import sys
import os
import subprocess
import _thread
from vmrunner import vmrunner

thread_timeout = 60

iperf_cmd = "iperf3"
transmit_size = "100M"

nsname="server1"

def move_tap1(o):
    subprocess.call(["./setup.sh", "--vmsetup"])

def clean():
    subprocess.call(["sudo","pkill",iperf_cmd])
    subprocess.call(["./setup.sh", "--clean"])

def iperf_server():
    subprocess.Popen(["sudo","ip","netns","exec", nsname, iperf_cmd, "-s"],
                                    stdout = subprocess.PIPE,
                                    stdin = subprocess.PIPE,
                                    stderr = subprocess.PIPE)

def iperf_client(o):
    print("Starting iperf client. Iperf output: ")
    print(subprocess.check_output([iperf_cmd,"-c","fe80:0:0:0:abcd:abcd:1234:8367%bridge43",
        "-n", transmit_size]))
    vmrunner.vms[0].exit(0, "Test completed without errors")
    return True

subprocess.call(["./setup.sh", "--clean"])
subprocess.call("./setup.sh")

vm = vmrunner.vms[0]

# Move second interface to second bridge, right after boot
vm.on_output("Routing test", move_tap1)


# Start iperf server right away, client when vm is up
_thread.start_new_thread(iperf_server, ())
vm.on_output("Service ready", iperf_client)

# Clean
vm.on_exit(clean)

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.cmake().boot(thread_timeout,image_name='net_router6').clean()
