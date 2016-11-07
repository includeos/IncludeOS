#!/usr/bin/python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src + "/test")

import vmrunner
vm = vmrunner.vms[0]
vm.make()

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
  print "num_outputs after increment: ", num_outputs

def check_num_outputs(line):
  assert(num_outputs == 18)
  vmrunner.vms[0].exit(0, "SUCCESS")

# POSIX syslog
vm.on_output("test_posix_syslog: " + INFO_C     + "<USER.INFO> "    + END_C + "No open has been called prior to this", increment)
vm.on_output("test_posix_syslog: " + NOTICE_C   + "<USER.NOTICE> "  + END_C + "Program created with two arguments: one and two", increment)
vm.on_output("Prepended message: " + ERR_C      + "<MAIL.ERR> "     + END_C + "Log after prepended message with one argument: 44", increment)
vm.on_output("Prepended message: " + WARNING_C  + "<MAIL.WARNING> " + END_C + "Log number two after openlog set prepended message", increment) 
vm.on_output("test_posix_syslog: " + WARNING_C  + "<USER.WARNING> " + END_C +
  "Log after closelog with three arguments. One is 33, another is this, a third is 4011", increment)
  # PID should be tested x2:
vm.on_output(EMERG_C + "<USER.EMERG> " + END_C + "Emergency log after openlog and new facility: user", increment)
vm.on_output(ALERT_C + "<USER.ALERT> " + END_C + "Alert log with the m argument: Success", increment)
vm.on_output("test_posix_syslog: " + CRIT_C + "<USER.CRIT> " + END_C + "Critical after cleared prepended message", increment)
vm.on_output("Open after close prepended message: " + INFO_C + "<USER.INFO> " + END_C +
  "Info after openlog with both m: Success and two hex arguments: 0x64 and 0x32", increment)

# IncludeOS syslogd
vm.on_output("test_posix_syslog: " + INFO_C     + "<USER.INFO> "    + END_C + "No <Syslogd> open has been called prior to this", increment)
vm.on_output("test_posix_syslog: " + NOTICE_C   + "<USER.NOTICE> "  + END_C + "Program <Syslogd> created with two arguments: one and two", increment)
vm.on_output("Prepended message: " + ERR_C      + "<MAIL.ERR> "     + END_C + "Log <Syslogd> after prepended message with one argument: 44", increment)
vm.on_output("Prepended message: " + WARNING_C  + "<MAIL.WARNING> " + END_C + "Log <Syslogd> number two after openlog set prepended message", increment) 
vm.on_output("test_posix_syslog: " + WARNING_C  + "<USER.WARNING> " + END_C +
  "Log <Syslogd> after closelog with three arguments. One is 33, another is this, a third is 4011", increment)
  # PID should be tested x2:
vm.on_output(EMERG_C + "<USER.EMERG> " + END_C + "Emergency <Syslogd> log after openlog and new facility: user", increment)
vm.on_output(ALERT_C + "<USER.ALERT> " + END_C + "Alert <Syslogd> log with the m argument: Success", increment)
vm.on_output("test_posix_syslog: " + CRIT_C + "<USER.CRIT> " + END_C + "Critical <Syslogd> after cleared prepended message", increment)
vm.on_output("Open after close prepended message: " + INFO_C + "<USER.INFO> " + END_C +
  "Info <Syslogd> after openlog with both m: Success and two hex arguments: 0x64 and 0x32", increment)

vm.on_output("Open after close prepended message: " + DEBUG_C + "<USER.DEBUG> " + END_C + "Something special to close with", check_num_outputs)

# Boot the VM, taking a timeout as parameter
vm.boot(30)