#! /usr/bin/env python

from __future__ import print_function
from builtins import str
import sys
import os

from vmrunner import vmrunner
vm = vmrunner.vms[0]

num_outputs = 0

EMERG_C   = "\033\\[38;5;1m"      # RED
ALERT_C   = "\033\\[38;5;160m"    # RED (lighter)
CRIT_C    = "\033\\[38;5;196m"    # RED (even lighter)
ERR_C     = "\033\\[38;5;208m"    # DARK YELLOW
WARNING_C = "\033\\[93m"          # YELLOW
NOTICE_C  = "\033\\[92m"          # GREEN
INFO_C    = "\033\\[96m"          # TURQUOISE
DEBUG_C   = "\033\\[94m"          # BLUE

END_C     = "\033\\[0m"           # CLEAR

def increment(line):
    global num_outputs
    num_outputs += 1
    print("num_outputs after increment: ", num_outputs)

def unexpected(line):
    assert False

expected_outputs = 22

def check_num_outputs(line):
    print("Registered", num_outputs, " / ", expected_outputs, " expected ouput lines")
    assert(num_outputs == expected_outputs)
    vmrunner.vms[0].exit(0, "All tests passed")

# ---------- POSIX wrapper syslog ----------
vm.on_output(" : Invalid -1", unexpected)

vm.on_output(" : A info message", increment)

vm.on_output(" : Program created with two arguments: one and two", increment)

vm.on_output(" Prepended message: Log after prepended message with one argument: 44", increment)

vm.on_output(" Prepended message: Log number two after openlog set prepended message", increment)

vm.on_output(" Prepended message: Log after closelog with three arguments. " +
  "One is 33, another is this, a third is 4011", increment)

vm.on_output(" Second prepended message\\[1\\]: Emergency log after openlog and new facility: user", increment)

vm.on_output(" Second prepended message\\[1\\]: Alert log with the m argument: Invalid argument", increment)

vm.on_output(" Second prepended message\\[1\\]: Second alert log with the m argument: No error information", increment)

vm.on_output(" Second prepended message\\[1\\]: Critical after cleared prepended message", increment)

# std err is just a regular printf
vm.on_output("Open after close prepended mess: " +
  "Info after openlog with both m: No error information and two hex arguments: 0x64 and 0x32", increment)

# ---------- IncludeOS syslogd ----------

# Count 1. vm.on_output("<11> " + ERR_C + "<USER.ERR> " + END_C, increment)
vm.on_output(" Syslog: Unknown priority -1. Message: Syslogd Invalid -1", increment)

# Count 1. vm.on_output("<11> " + ERR_C + "<USER.ERR> " + END_C, increment)
vm.on_output(" Syslog: Unknown priority 10. Message: Syslogd Invalid 10", increment)

# Count 1. vm.on_output("<11> " + ERR_C + "<USER.ERR> " + END_C, increment)
vm.on_output(" Syslog: Unknown priority 55. Message: Syslogd Invalid 55", increment)

# Count 1. vm.on_output("<14> " + INFO_C + "<USER.INFO> " + END_C, increment)
vm.on_output(" Syslogd No open has been called prior to this", increment)

# Count 1. vm.on_output("<13> " + NOTICE_C + "<USER.NOTICE> " + END_C, increment)
vm.on_output(" Syslogd Program created with two arguments: one and two", increment)

# Count 1. vm.on_output("<19> " + ERR_C + "<MAIL.ERR> " + END_C, increment)
vm.on_output(" Prepended message: Syslogd Log after prepended message with one argument: 44", increment)

# Count 1. vm.on_output("<20> " + WARNING_C  + "<MAIL.WARNING> " + END_C, increment)
vm.on_output(" Prepended message: Syslogd Log number two after openlog set prepended message", increment)

# Count 1. vm.on_output("<12> " + WARNING_C  + "<USER.WARNING> " + END_C, increment)
vm.on_output(" Syslogd Log after closelog with three arguments. " +
  "One is 33, another is this, a third is 4011", increment)

# Count 1. vm.on_output("<8> " + EMERG_C + "<USER.EMERG> " + END_C, increment)
vm.on_output(" Second prepended message\\[1\\]: Syslogd Emergency log after openlog and new facility: user", increment)

# Count 1. vm.on_output("<9> "  + ALERT_C + "<USER.ALERT> " + END_C, increment)
vm.on_output(" Second prepended message\\[1\\]: Syslogd Alert log with the m argument: Success", increment)

# Count 1. vm.on_output("<10> " + CRIT_C + "<USER.CRIT> " + END_C, increment)
vm.on_output(" Syslogd Critical after cleared prepended message", increment)

# Count 2. Also has logopt LOG_PERROR (so will also be written to std::cerr)
# Count 1. vm.on_output("<6> " + INFO_C + "<KERN.INFO> " + END_C, increment)
vm.on_output(" Open after close prepended message: " +
  "Syslogd Info after openlog with both m: No error information and two hex arguments: 0x64 and 0x32", increment)

vm.on_output("<191> " + DEBUG_C + "<LOCAL7.DEBUG> " + END_C, increment)
vm.on_output(" Exiting test: Something special to close with", check_num_outputs)

# Boot the VM, taking a timeout as parameter
# Boot the VM, taking a timeout as parameter
if len(sys.argv) > 1:
    vm.boot(20,image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(20,image_name='posix_syslog_default').clean()
