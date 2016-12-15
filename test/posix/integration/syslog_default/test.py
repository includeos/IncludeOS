#! /usr/bin/env python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src)

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
  print "num_outputs after increment: ", num_outputs

def check_num_outputs(line):
  assert(num_outputs == 53)
  vmrunner.vms[0].exit(0, "SUCCESS")

# ---------- POSIX wrapper syslog ----------

vm.on_output("<11> " + ERR_C + "<USER.ERR> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslog: Unknown priority -1. Message: Invalid -1", increment)

# Count 1. vm.on_output("<11> " + ERR_C + "<USER.ERR> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslog: Unknown priority 10. Message: Invalid 10", increment)

# Count 1. vm.on_output("<11> " + ERR_C + "<USER.ERR> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslog: Unknown priority 55. Message: Invalid 55", increment)

vm.on_output("<14> " + INFO_C + "<USER.INFO> " + END_C, increment)
vm.on_output(" test_syslog_default: No open has been called prior to this", increment)

vm.on_output("<13> " + NOTICE_C + "<USER.NOTICE> " + END_C, increment)
vm.on_output(" test_syslog_default: Program created with two arguments: one and two", increment)

vm.on_output("<19> " + ERR_C + "<MAIL.ERR> " + END_C, increment)
vm.on_output(" test_syslog_default Prepended message: Log after prepended message with one argument: 44", increment)

vm.on_output("<20> " + WARNING_C  + "<MAIL.WARNING> " + END_C, increment)
vm.on_output(" test_syslog_default Prepended message: Log number two after openlog set prepended message", increment)

vm.on_output("<12> " + WARNING_C  + "<USER.WARNING> " + END_C, increment)
vm.on_output(" test_syslog_default: Log after closelog with three arguments. " +
  "One is 33, another is this, a third is 4011", increment)

vm.on_output("<8> " + EMERG_C + "<USER.EMERG> " + END_C, increment)
vm.on_output(" test_syslog_default Second prepended message\\[1\\]: Emergency log after openlog and new facility: user", increment)

vm.on_output("<9> "  + ALERT_C + "<USER.ALERT> " + END_C, increment)
vm.on_output(" test_syslog_default Second prepended message\\[1\\]: Alert log with the m argument: Success", increment)

vm.on_output("<10> " + CRIT_C + "<USER.CRIT> " + END_C, increment)
vm.on_output(" test_syslog_default: Critical after cleared prepended message", increment)

# Count 2. Also has logopt LOG_PERROR (so will also be written to std::cerr)
vm.on_output("<6> " + INFO_C + "<KERN.INFO> " + END_C, increment)
vm.on_output(" test_syslog_default Open after close prepended message: " +
  "Info after openlog with both m: Success and two hex arguments: 0x64 and 0x32", increment)

# ---------- IncludeOS syslogd ----------

# Count 1. vm.on_output("<11> " + ERR_C + "<USER.ERR> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslog: Unknown priority -1. Message: Syslogd Invalid -1", increment)

# Count 1. vm.on_output("<11> " + ERR_C + "<USER.ERR> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslog: Unknown priority 10. Message: Syslogd Invalid 10", increment)

# Count 1. vm.on_output("<11> " + ERR_C + "<USER.ERR> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslog: Unknown priority 55. Message: Syslogd Invalid 55", increment)

# Count 1. vm.on_output("<14> " + INFO_C + "<USER.INFO> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslogd No open has been called prior to this", increment)

# Count 1. vm.on_output("<13> " + NOTICE_C + "<USER.NOTICE> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslogd Program created with two arguments: one and two", increment)

# Count 1. vm.on_output("<19> " + ERR_C + "<MAIL.ERR> " + END_C, increment)
vm.on_output(" test_syslog_default Prepended message: Syslogd Log after prepended message with one argument: 44", increment)

# Count 1. vm.on_output("<20> " + WARNING_C  + "<MAIL.WARNING> " + END_C, increment)
vm.on_output(" test_syslog_default Prepended message: Syslogd Log number two after openlog set prepended message", increment)

# Count 1. vm.on_output("<12> " + WARNING_C  + "<USER.WARNING> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslogd Log after closelog with three arguments. " +
  "One is 33, another is this, a third is 4011", increment)

# Count 1. vm.on_output("<8> " + EMERG_C + "<USER.EMERG> " + END_C, increment)
vm.on_output(" test_syslog_default Second prepended message\\[1\\]: Syslogd Emergency log after openlog and new facility: user", increment)

# Count 1. vm.on_output("<9> "  + ALERT_C + "<USER.ALERT> " + END_C, increment)
vm.on_output(" test_syslog_default Second prepended message\\[1\\]: Syslogd Alert log with the m argument: Success", increment)

# Count 1. vm.on_output("<10> " + CRIT_C + "<USER.CRIT> " + END_C, increment)
vm.on_output(" test_syslog_default: Syslogd Critical after cleared prepended message", increment)

# Count 2. Also has logopt LOG_PERROR (so will also be written to std::cerr)
# Count 1. vm.on_output("<6> " + INFO_C + "<KERN.INFO> " + END_C, increment)
vm.on_output(" test_syslog_default Open after close prepended message: " +
  "Syslogd Info after openlog with both m: Success and two hex arguments: 0x64 and 0x32", increment)

vm.on_output("<191> " + DEBUG_C + "<LOCAL7.DEBUG> " + END_C, increment)
vm.on_output(" test_syslog_default Exiting test: Something special to close with", check_num_outputs)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(20).clean()
