#! /usr/bin/env python

import sys
import os
import subprocess
import thread

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner

if1 = "tap0"
if2 = "tap1"
br1 = "bridge43"
br2 = "bridge44"

iperf_cmd = "iperf3"
iperf_server_proc = None
transmit_size = "100M"

nsname="server1"

def move_tap1(o):
    print "Moving",if2, "to", br2
    subprocess.call(["sudo", "brctl", "delif", br1, if2])
    subprocess.call(["sudo", "brctl", "addif", br2, if2])
    subprocess.call(["sudo", "ifconfig", if2, "up"])


def clean():
    subprocess.call(["sudo","pkill",iperf_cmd])
    subprocess.call(["./setup.sh", "--clean"])


def iperf_server():
    global iperf_server_proc, iperf_srv_log
    iperf_server_proc = subprocess.Popen(["sudo","ip","netns","exec", nsname, iperf_cmd, "-s"],
                                    stdout = subprocess.PIPE,
                                    stdin = subprocess.PIPE,
                                    stderr = subprocess.PIPE)

def iperf_client(o):
    print "Starting iperf client. Iperf output: "
    print subprocess.check_output([iperf_cmd,"-c","fe80:0:0:0:abcd:abcd:1234:8367%bridge43",
        "-n", transmit_size])
    vmrunner.vms[0].exit(0, "Test completed without errors")
    return True


subprocess.call("./setup.sh")

vm = vmrunner.vms[0]

# Move second interface to second bridge, right after boot
# TODO: Add support for per-interface qemu-ifup scripts instead?
vm.on_output("Routing test service", move_tap1)


# Start iperf server right away, client when vm is up
thread.start_new_thread(iperf_server, ())
vm.on_output("Service ready", iperf_client)


# Clean
vm.on_exit(clean)

# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.cmake().boot(30,image_name='net_router6').clean()
