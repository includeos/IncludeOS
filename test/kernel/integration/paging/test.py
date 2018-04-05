#! /usr/bin/env python
import sys
import os
import socket

includeos_src = os.environ.get('INCLUDEOS_SRC',
           os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
vm = vmrunner.vms[0]

expected_read_fail = 3
expected_write_fail = 1
expected_exec_fail = 2
expected_boots = 5
expected_others = 3

boot_count = 0
read_fails = 0
write_fails = 0
exec_fails = 0
others = 0

def booted(line):
    global boot_count
    boot_count += 1;
    print "Booted ", boot_count,"/",expected_boots

def exec_fail(line):
    global exec_fails
    exec_fails += 1;
    print "Execute failure ", exec_fails, "/", expected_exec_fail

def read_fail(line):
    global read_fails
    read_fails += 1
    print "Page read fail ", read_fails, "/", expected_read_fail

def write_fail(line):
    global write_fails
    write_fails += 1
    print "Page write fail ", write_fails, "/", expected_write_fail

def other(line):
    global others
    others += 1

def expected_cases():
    return expected_read_fail
+ expected_write_fail
+ expected_exec_fail
+ expected_boots
+ expected_others

def recorded_cases():
    return read_fails + write_fails + exec_fails + boot_count + others

def done(line):
    print "Test summary: "
    print "VM boots: ", boot_count
    print "Read fails: ", read_fails
    print "Write fails: ", write_fails
    print "Exec fails: ", exec_fails
    print "Others: ", others

    if (read_fails == expected_read_fail and
        write_fails == expected_write_fail and
        exec_fails == expected_exec_fail and
        boot_count == expected_boots and
        others == expected_others):
        vm.exit(0, "All tests passed")
    else:
        vm.exit(1, "Expected " + str(expected_cases())
                + " cases recorded " + str(recorded_cases()))


vm.on_output("#include<os> // Literally", booted);
vm.on_output("Page write failed.", write_fail)
vm.on_output("Page read failed", read_fail)
vm.on_output("Instruction fetch. XD", exec_fail)

vm.on_output("1 WRITE protection PASSED", other)
vm.on_output("2 READ protection PASSED", other)
vm.on_output("3 EXECUTE protection 1/2 PASSED", other)
vm.on_output("4 EXECUTE protection 2/2 PASSED", done)

vm.cmake().boot(20).clean()
