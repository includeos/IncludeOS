#! /usr/bin/env python
from __future__ import division
from __future__ import print_function
from builtins import str
from builtins import range
from past.utils import old_div
import sys
import socket
import time
import subprocess
import subprocess32
import os

from vmrunner import vmrunner
from vmrunner.prettify import color

test_name="Stresstest"
name_tag = "<" + test_name + ">"

# We assume malloc will increase / decrease heap pagewise
PAGE_SIZE = 4096

BURST_SIZE = 1000
BURST_COUNT = 10
BURST_INTERVAL = 0.1

HOST = "10.0.0.42"
PORT_FLOOD = 4242
PORT_MEM = 4243
memuse_at_start = 0
sock_timeout = 20

# Boot the VM, taking a timeout as parameter
thread_timeout = BURST_COUNT * 30

# It's to be expected that the VM allocates more room during the running of tests
# e.g. for containers, packets etc. These should all be freed after a run.
acceptable_increase = 12 * PAGE_SIZE

# A persistent connection to the VM for getting memory info
# TODO: This should be expanded to check more vital signs, such as time of day,
# connection / packet statistics (>= what we have sent) etc.
sock_mem = socket.socket
sock_mem = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

heap_verified = False

def get_mem():
  name_tag = "<" + test_name + "::get_mem>"

  try:
    # We expect this socket to allready be opened
    time.sleep(1)
    sock_mem.send("memsize\n".encode())
    received = sock_mem.recv(1000).rstrip()

  except Exception as e:
    print(color.FAIL(name_tag), "Python socket failed while getting memsize: ", e)
    return False

  print(color.INFO(name_tag),"Current VM memory usage reported as ", received)
  return int(received)

def get_mem_start():
  global memuse_at_start
  if memuse_at_start == 0:
      memuse_at_start = get_mem()
  return memuse_at_start

def memory_increase(lead_time, expected_memuse = memuse_at_start):
  name_tag = "<" + test_name + "::memory_increase>"
  if lead_time:
    print(color.INFO(name_tag),"Checking for memory increase after a lead time of ",lead_time,"s.")
    # Give the VM a chance to free up resources before asking
    time.sleep(lead_time)

  use = get_mem()
  increase = use - expected_memuse
  percent = 0.0;
  if (increase):
    percent = float(increase) / expected_memuse

  if increase > acceptable_increase:
    print(color.WARNING(name_tag), "Memory increased by ", percent, "%.")
    print("(" , expected_memuse, "->", use, ",", increase,"b increase, but no increase expected.)")
  else:
    print(color.OK(name_tag + "Memory constant, no leak detected"))
  return increase

# Fire a single burst of UDP packets
def UDP_burst(burst_size = BURST_SIZE, burst_interval = BURST_INTERVAL):
  global memuse_at_start
  sock = socket.socket
  # SOCK_DGRAM is the socket type to use for UDP sockets
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.settimeout(sock_timeout)
  # This is a stress-test, so we don't want to spend time checking the output
  # (especially not with python)
  # We just want to make sure the VM survives.
  data = "UDP is working as it's supposed to."

  try:
    for i in range(0, burst_size):
      sock.sendto(data, (HOST, PORT_FLOOD))
  except Exception as e:
    print(color.WARNING("<Test.py> Python socket timed out while sending. "))
    return False
  sock.close()
  time.sleep(burst_interval)
  return get_mem()

# Fire a single burst of ICMP packets
def ICMP_flood(burst_size = BURST_SIZE, burst_interval = BURST_INTERVAL):
  # Note: Ping-flooding requires sudo for optimal speed
  res = subprocess32.check_call(["sudo","ping","-f", HOST, "-c", str(burst_size)], timeout=thread_timeout);
  time.sleep(burst_interval)
  return get_mem()

# Fire a single burst of HTTP requests
def httperf(burst_size = BURST_SIZE, burst_interval = BURST_INTERVAL):
  res = subprocess32.check_call(["httperf","--hog", "--server", HOST, "--num-conn", str(burst_size)], timeout=thread_timeout);
  time.sleep(burst_interval)
  return get_mem()

# Fire a single burst of ARP requests
def ARP_burst(burst_size = BURST_SIZE, burst_interval = BURST_INTERVAL):
  # Note: Arping requires sudo, and we expect the bridge 'bridge43' to be present
  command = ["sudo", "arping", "-q","-W", str(0.0001), "-I", "bridge43", "-c", str(burst_size * 10),  HOST]
  print(color.DATA(" ".join(command)))
  time.sleep(0.5)
  res = subprocess32.check_call(command, timeout=thread_timeout);
  time.sleep(burst_interval)
  return get_mem()



def heap_ok(line):
    global heap_verified
    heap_verified = True
    print(color.INFO("Stresstest::heap_ok"), "VM reports heap is increasing and decreasing as expected")


def crash_test(string):
  print(color.INFO("Opening persistent TCP connection for diagnostics"))
  sock_mem.connect((HOST, PORT_MEM))
  mem_before = get_mem_start()
  if mem_before <= 0:
      print(color.FAIL("Initial memory reported as " + str(mem_before)))
      return False

  if not heap_verified:
      print(color.FAIL("Heap behavior was not verified as expected. "))
      return False

  print(color.HEADER("Initial crash test"))
  burst_size = BURST_SIZE * 10

  ARP_burst(burst_size, 0)
  UDP_burst(burst_size, 0)
  ICMP_flood(burst_size, 0)
  httperf(burst_size, 0)
  time.sleep(BURST_INTERVAL)
  mem_after = get_mem()
  print(color.INFO("Crash test complete. Memory in use: "), mem_after)
  return mem_after >= memuse_at_start

# Fire several bursts, e.g. trigger a function that fires bursts, several times
def fire_bursts(func, sub_test_name, lead_out = 3):
  name_tag = "<" + sub_test_name + ">"
  print(color.HEADER(test_name + " initiating "+sub_test_name))
  membase_start = get_mem()
  mem_base = membase_start

  # Track heap behavior
  increases = 0
  decreases = 0
  constant = 0

  for i in range(0,BURST_COUNT):
    print(color.INFO(name_tag), " Run ", i+1)
    memi = func()
    if memi > mem_base:
      memincrease =  memi - mem_base
      increases += 1
    elif memi == mem_base:
      memincrease = 0
      constant += 1
    else:
      memincrease = 0
      decreases += 1

    # We want to know how much each burst increases memory relative to the last burst
    mem_base = memi

    if memincrease > acceptable_increase:
      print(color.WARNING(name_tag), "Memory increased by ",memincrease,"b, ",float(memincrease) / BURST_SIZE, "pr. packet \n")
    else:
      print(color.OK(name_tag), "Memory increase ",memincrease,"b \n")

    # Memory can decrease, we don't care about that
    # if memincrease > 0:
    #  mem_base += memincrease
  print(color.INFO(name_tag),"Heap behavior: ", "+",increases, ", -",decreases, ", ==", constant)
  print(color.INFO(name_tag),"Done. Checking for liveliness")
  if memory_increase(lead_out, membase_start) > acceptable_increase:
    print(color.FAIL(sub_test_name + " failed "))
    return False
  print(color.PASS(sub_test_name + " succeeded "))
  return True


# Trigger several UDP bursts
def ARP(string):
  return fire_bursts(ARP_burst, "ARP bombardment")

# Trigger several UDP bursts
def UDP(string):
  return fire_bursts(UDP_burst, "UDP bombardment")

# Trigger several ICMP bursts
def ICMP(string):
  return fire_bursts(ICMP_flood, "Ping-flooding");

# Trigger several HTTP-brusts
def TCP(string):
  return fire_bursts(httperf, "HTTP bombardment")


# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Check for vital signs after all the bombardment is done
def check_vitals(string):
  print(color.INFO("Checking vital signs"))
  mem = get_mem()
  diff = mem - memuse_at_start
  pages = old_div(diff, PAGE_SIZE)
  print(color.INFO("Memory use at test end:"), mem, "bytes")
  print(color.INFO("Memory difference from test start:"), memuse_at_start, "bytes (Diff:",diff, "b == ",pages, "pages)")
  sock_mem.close()
  vm.stop()
  wait_for_tw()
  return True

# Wait for sockets to exit TIME_WAIT status
def wait_for_tw():
  print(color.INFO("Waiting for sockets to clear TIME_WAIT stage"))
  socket_limit = 11500
  time_wait_proc = 30000
  while time_wait_proc > socket_limit:
    output = subprocess.check_output(('netstat', '-anlt')).decode("utf-8")
    output = output.split('\n')
    time_wait_proc = 0
    for line in output:
        if "TIME_WAIT" in line:
            time_wait_proc += 1
    print(color.INFO("There are {0} sockets in use, waiting for value to drop below {1}".format(time_wait_proc, socket_limit)))
    time.sleep(7)

# Add custom event-handlers
vm.on_output("Heap functioning as expected", heap_ok)
vm.on_output("Ready to start", crash_test)
vm.on_output("Ready for ARP", ARP)
vm.on_output("Ready for UDP", UDP)
vm.on_output("Ready for ICMP", ICMP)
vm.on_output("Ready for TCP", TCP)
vm.on_output("Ready to end", check_vitals)

if len(sys.argv) > 1:
  thread_timeout = int(sys.argv[1])

if len(sys.argv) > 3:
  BURST_COUNT = int(sys.argv[2])
  BURST_SIZE = int(sys.argv[3])

print(color.HEADER(test_name + " initializing"))
print(color.INFO(name_tag),"configured for ", BURST_COUNT,"bursts of", BURST_SIZE, "packets each")

if len(sys.argv) > 4:
    vm.boot(timeout=thread_timeout, image_name=str(sys.argv[4]))
else:
    vm.cmake().boot(thread_timeout).clean()
