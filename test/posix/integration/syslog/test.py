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

import socket

# Gateway IP is 10.0.0.1 - syslog sends its messages here on port 6000 (alternatively 514 - the syslog port)

UDP_IP = "10.0.0.1"
UDP_PORT = 6000 # 514 alternatively
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

num_received = 0

msg1_part1 = "<11>1 "
msg1_part2 = " 10.0.0.45 test_posix_syslog 1 UDPOUT - Syslog: Unknown priority -1. Message: Invalid -1"

msg2_part1 = "<11>1 "
msg2_part2 = " 10.0.0.45 test_posix_syslog 1 UDPOUT - Syslog: Unknown priority 10. Message: Invalid 10"

msg3_part1 = "<11>1 "
msg3_part2 = " 10.0.0.45 test_posix_syslog 1 UDPOUT - Syslog: Unknown priority 55. Message: Invalid 55"

messages = [msg1_part1 + msg1_part2,
            msg2_part1 + msg2_part2,
            msg3_part1 + msg3_part2]

end_msg = "Something special to close with"

def increment():
  global num_received
  num_received += 1
  print "num_received after increment: ", num_received

def validate(data):
  print "Received message: ", data
  # Remove first condition when filled in all test cases in messages-array
  if num_received >= len(messages) or messages[num_received - 1] in data:
    return True
  else:
    return False

def start(line):
  while True:
    data, addr = sock.recvfrom(4096)
    increment()

    if end_msg not in data:
      validate(data)
    else:
      #assert(num_received == )
      end()

def end():
  sock.close()
  vmrunner.vms[0].exit(0, "SUCCESS")

vm.on_output("Service IP address is 10.0.0.45", start)

''' Previously (when message format was not according to RFC5424):
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
'''

# Boot the VM, taking a timeout as parameter
vm.boot(30)