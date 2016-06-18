#! /usr/bin/python
import sys
import socket
import time
import subprocess
sys.path.insert(0,"..")

import vmrunner

COUNT = 10000
HOST = "10.0.0.42"
PORT_FLOOD = 4242
PORT_MEM = 4243
memuse_at_start = 0
sock_timeout = 20

# It's to be expected that the VM allocates more room during the running of tests
# However, we don't expect running the same test a second time to further increase memory
# (i.e. the space that was allocated is now free for another run)
acceptable_increase = 1

def get_mem():
  sock = socket.socket
  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

  try:
    sock.connect((HOST, PORT_MEM))
    sock.send("memsize\n")
    received = sock.recv(1000).rstrip()

  except Exception as e:
    print "<Test.py: get_mem> Python socket failed while getting memsize: ",e
    return False

  print "<Test.py> Current VM memory usage reported as ",received
  sock.close()
  return int(received)

def get_mem_start():
  global memuse_at_start
  memuse_at_start = get_mem()

def memory_increase(lead_time, expected_memuse = memuse_at_start):

  if lead_time:
    print "<Test.py> Checking for memory increase after a lead time of ",lead_time,"s."
    # Give the VM a chance to free up resources before asking
    time.sleep(lead_time)

  use = get_mem()
  increase = use - expected_memuse
  percent = 0.0;
  if (increase):
    percent = increase / expected_memuse

  if increase > acceptable_increase:
    print "<Test.py> Memory increased by ", percent, "% (",expected_memuse,"->",use," + ",increase,"b). It should remain constant"
  else:
    print "<Test.py> Memory constant, no leak detected"
  return increase

def UDP_bombardment():
  global memuse_at_start
  sock = socket.socket
  # SOCK_DGRAM is the socket type to use for UDP sockets
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.settimeout(sock_timeout)
  print "<Test.py: UDP> Performing UDP bombardment"

  # This is a stress-test, so we don't want to spend time checking the output
  # (especially not with python)
  # We just want to make sure the VM survives.
  data = "UDP is working as it's supposed to."

  try:
    for i in range(0,COUNT):
      sock.sendto(data, (HOST, PORT_FLOOD))
  except Exception as e:
    print "<Test.py> Python socket timed out while sending. "
    return False
  sock.close()
  time.sleep(1)
  return get_mem()

def UDP_test():

  membase_start = UDP_bombardment()
  mem_base = membase_start
  for i in range(0,10):
    print "<Test.py: UDP> Run ",i+1
    memi = UDP_bombardment()
    memincrease =  memi - mem_base
    print "<Test.py: UDP> Memory increased by ",memincrease, "bytes, ", float(memincrease) / COUNT, "pr. packet"
    mem_base = memi

  print "<Test.py> UDP done. Checking for liveliness"
  if memory_increase(3, membase_start) > acceptable_increase:  return False
  print "<Test.py> UDP bombardment succeeded \n"


def ICMP_flood():
  print "<Test.py> Doing ICMP flooding"
  # Note: Ping-flooding requires sudo for optimal speed

  # Run 1
  res = subprocess.check_call(["sudo","ping","-f", HOST, "-c", str(COUNT)]);
  time.sleep(1)
  mem_run1 = get_mem()

  # Run 2
  res = subprocess.check_call(["sudo","ping","-f", HOST, "-c", str(COUNT)]);

  if memory_increase(3, mem_run1)["pct"]:  return False
  print "<Test.py> ICMP flooding succeeded \n"

def httperf():
  print "<Test.py: HTTP> Doing httperf"

  # Run 1
  res = subprocess.check_call(["httperf","--hog", "--server", HOST, "--num-conn", str(COUNT)]);
  time.sleep(1)
  mem_run1 = get_mem()

  # Run 2
  res = subprocess.check_call(["httperf","--hog", "--server", HOST, "--num-conn", str(COUNT)]);

  if memory_increase(1, mem_run1)["pct"]:  return False
  print "<Test.py: HTTP> HTTP flooding succeeded \n"


# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event-handler
vm.on_output("Ready to start", get_mem_start)
vm.on_output("Ready for UDP", UDP_test)
vm.on_output("Ready for ICMP", ICMP_flood)
vm.on_output("Ready for TCP", httperf)

# Boot the VM, taking a timeout as parameter
timeout = 60

if len(sys.argv) > 1:
  timeout = int(sys.argv[1])

vm.boot(timeout)
