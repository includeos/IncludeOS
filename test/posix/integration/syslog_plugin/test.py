#! /usr/bin/env python
import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
print 'includeos_src: {0}'.format(includeos_src)
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
vm = vmrunner.vms[0]

import socket

# Gateway IP is 10.0.0.1 - syslog sends its messages here on port 6514

UDP_IP = "10.0.0.1"
UDP_PORT = 6514
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

num_received = 0
num_expected_msgs = 24

pre_msg1 = "<11>1 "
post_msg1 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - Syslog: Unknown priority -1. Message: Invalid -1"

pre_msg2 = "<11>1 "
post_msg2 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - Syslog: Unknown priority 10. Message: Invalid 10"

pre_msg3 = "<11>1 "
post_msg3 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - Syslog: Unknown priority 55. Message: Invalid 55"

pre_msg4 = "<14>1 "
post_msg4 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - (Info) No open has been called prior to this"

pre_msg5 = "<13>1 "
post_msg5 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - (Notice) Program created with two arguments: one and two"

pre_msg6 = "<19>1 "
post_msg6 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - Prepended message (Err) Log after prepended message with " + \
"one argument: 44"

pre_msg7 = "<20>1 "
post_msg7 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - Prepended message (Warning) Log number two after openlog " + \
"set prepended message"

pre_msg8 = "<12>1 "
post_msg8 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - (Warning) Log after closelog with three arguments. One is 33, " + \
"another is this, a third is 4011"

pre_msg9 = "<8>1 "
post_msg9 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - Second prepended message Emergency log after openlog and new " + \
"facility: user"

pre_msg10 = "<9>1 "
post_msg10 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - Second prepended message Alert log with the m argument: Success"

pre_msg11 = "<10>1 "
post_msg11 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - Critical after cleared prepended message (closelog has been called)"

pre_msg12 = "<22>1 "
post_msg12 = " 10.0.0.46 test_syslog_plugin 1 UDPOUT - Open after close prepended message Info after openlog with " + \
"both m: Success and two hex arguments: 0x64 and 0x32"

pre_messages = [pre_msg1, pre_msg2, pre_msg3, pre_msg4, pre_msg5, pre_msg6, pre_msg7, pre_msg8,
                pre_msg9, pre_msg10, pre_msg11, pre_msg12]

post_messages = [post_msg1, post_msg2, post_msg3, post_msg4, post_msg5, post_msg6, post_msg7, post_msg8,
                post_msg9, post_msg10, post_msg11, post_msg12]

end_msg = "Something special to close with"

def increment():
  global num_received
  num_received += 1
  print "num_received after increment: ", num_received

def validate(data):
  print "Received message: ", data
  if num_received < len(pre_messages):
    assert pre_messages[num_received] in data and post_messages[num_received], "Message don't match"
  else:
    assert pre_messages[num_received - len(pre_messages)] in data and \
    post_messages[num_received - len(pre_messages)], "Message don't match"

def start(line):
    print "Waiting for UDP data"
    while True:
        data, addr = sock.recvfrom(4096)
        print "Received data"
        if end_msg not in data:
            validate(data)
            increment()
        else:
            assert(num_received == num_expected_msgs)
            end()
            return True

def end():
    sock.close()
    vmrunner.vms[0].exit(0, "All expected syslog messages received")

vm.on_output("Service IP address is 10.0.0.46", start)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(60).clean()
