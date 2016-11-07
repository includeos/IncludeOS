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

def increment(line):
  global num_outputs
  num_outputs += 1
  print "num_outputs after increment: ", num_outputs

def check_num_outputs(line):
  assert(num_outputs == 18)
  vmrunner.vms[0].exit(0, "SUCCESS")

# Would also like to test ident (prepended message: ) and facility and priority (<MAIL.WARNING>)
# The colors are standing in the way

# POSIX syslog
vm.on_output("No open has been called prior to this", increment)
vm.on_output("Program created with two arguments: one and two", increment)
vm.on_output("Log after prepended message with one argument: 44", increment)
vm.on_output("Log number two after openlog set prepended message", increment) 
vm.on_output("Log after closelog with three arguments. One is 33, another is this, a third is 4011", increment)
vm.on_output("Emergency log after openlog and new facility: user", increment)
vm.on_output("Alert log with the m argument: Success", increment)
vm.on_output("Critical after cleared prepended message", increment)
vm.on_output("Info after openlog with both m: Success and two hex arguments: 0x64 and 0x32", increment)

# IncludeOS syslogd
vm.on_output("No <Syslogd> open has been called prior to this", increment)
vm.on_output("Program <Syslogd> created with two arguments: one and two", increment)
vm.on_output("Log <Syslogd> after prepended message with one argument: 44", increment)
vm.on_output("Log <Syslogd> number two after openlog set prepended message", increment) 
vm.on_output("Log <Syslogd> after closelog with three arguments. One is 33, another is this, a third is 4011", increment)
vm.on_output("Emergency <Syslogd> log after openlog and new facility: user", increment)
vm.on_output("Alert <Syslogd> log with the m argument: Success", increment)
vm.on_output("Critical <Syslogd> after cleared prepended message", increment)
vm.on_output("Info <Syslogd> after openlog with both m: Success and two hex arguments: 0x64 and 0x32", increment)

vm.on_output("Something special to close with", check_num_outputs)

# Boot the VM, taking a timeout as parameter
vm.boot(30)