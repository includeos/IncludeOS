print "vmrunner.py running"

import os
import sys
import subprocess
import threading
import time
print os.getcwd()
import re

import validate_test

def abstract():
    raise Exception("Abstract class method called. Use a subclass")
# Hypervisor base / super class
# (It seems to be recommended for "new style classes" to inherit object)
class hypervisor(object):

  def __init__(self, vm):
    self._vm = vm;

  # Boot a VM, returning a hypervisor handle for reuse
  def boot(self):
    abstract()

  # Stop the VM booted by boot
  def stop(self):
    abstract()

  # Read a line of output from vm
  def readline(self):
    abstract()

  # Verify that the hypervisor is available
  def available(self, config_data = None):
    abstract()

  # Wait for this VM to exit
  def wait(self):
    abstract()

  # Wait for this VM to exit
  def poll(self):
    abstract()

# Start a process we expect to not finish immediately (i.e. a VM)
def start_process(popen_param_list):
  # Start a subprocess
  proc = subprocess.Popen(popen_param_list,
                          stdout = subprocess.PIPE,
                          stderr = subprocess.PIPE)

  # After half a second it should be started, otherwise throw
  time.sleep(0.5)
  if (proc.poll()):
    data, err = proc.communicate()
    raise Exception("Process exited. ERROR: " + err + " " + data);

  print "<VMRunner> Started process PID ",proc.pid
  return proc


# Qemu Hypervisor interface
class qemu(hypervisor):

  def drive_arg(self, filename, drive_type="virtio", drive_format="raw"):
    return ["-drive","file="+filename+",format="+drive_format+",if="+drive_type]

  def net_arg(self, if_type = "virtio", if_name = "net0", mac="c0:01:0a:00:00:2a"):
    type_names = {"virtio" : "virtio-net"}
    qemu_ifup = os.environ['INCLUDEOS_HOME']+"/etc/qemu-ifup"
    return ["-device", type_names[if_type]+",netdev="+if_name+",mac="+mac,
            "-netdev", "tap,id="+if_name+",script="+qemu_ifup]


  def boot(self):
    self._out_sign = "<" + type(self).__name__ + ">"
    print self._out_sign,"booting ",self._vm["image"]

    disk_args = self.drive_arg(self._vm["image"], "ide")
    if self._vm.has_key("drives"):
      for disk in self._vm["drives"]:
        disk_args += drive_arg(disk["file"], disk["type"], disk["format"])

    net_args = []
    i = 0
    if self._vm.has_key("net"):
      for net in self._vm["net"]:
        net_args += self.net_arg(net["type"], "net"+str(i), net["mac"])
        i+=1

    command = ["sudo", "qemu-system-x86_64", "-nographic" ] + disk_args + net_args
    print self._out_sign, "command:", command

    self._proc = start_process(command)

  def stop(self):
    if hasattr(self, "_proc") and self._proc.poll() == None :
      print self._out_sign,"Stopping", self._vm["image"], "PID",self._proc.pid
      # Kill with sudo
      subprocess.check_call(["sudo","kill", "-SIGTERM", str(self._proc.pid)])
      # Wait for termination (avoids the need to reset the terminal)
      self._proc.wait()

  def wait(self):
    print "Waiting for process to terminate"
    self._proc.wait()

  def readline(self):
    if self._proc.poll():
      raise Exception("Process completed")
    return self._proc.stdout.readline()

  def poll(self):
    return self._proc.poll()

# VM class
class vm:

  def __init__(self, config, hyper = qemu):
    self._exit_status = 0
    self._config = config
    self._on_success = lambda : self.exit(0, "<VMRun> SUCCESS : All tests passed")
    self._on_panic =  lambda : self.exit(66, "<VMRun> FAIL : " + self._hyper.readline())
    self._on_timeout = lambda : self.exit(67, "<VMRun> TIMEOUT: Test timed out")
    self._on_output = {
      "PANIC" : self._on_panic,
      "SUCCESS" : self._on_success }
    assert(issubclass(hyper, hypervisor))
    self._hyper  = hyper(config)
    self._timer = None

  def exit(self, status, msg):
    self.stop()
    print msg
    self._exit_status = status
    sys.exit(status)

  def on_output(self, output, callback):
    self._on_output[ output ] = callback

  def on_success(self, callback):
    self._on_success = callback

  def on_panic(self, callack):
    self._on_panic = callback

  def on_timeout(self, callback):
    self._on_timeout = callback

  def boot(self, timeout = None):

    # Start the timeout thread
    if (timeout):
      self._timer = threading.Timer(timeout, self._on_timeout)
      self._timer.start()

    # Boot via hypervisor
    try:
      self._hyper.boot()
    except Exception as err:
      self._timer.cancel()
      raise err
      #self.exit(1, err)

    # Start analyzing output
    while self._hyper.poll() == None and not self._exit_status:
      line = self._hyper.readline()
      print "<VM>",line.rstrip()

      # Look for event-triggers
      for pattern, func in self._on_output.iteritems():
        if re.search(pattern, line):
          func()
    return self

  def stop(self):
    self._hyper.stop()
    if hasattr(self, "_timer") and self._timer:
      self._timer.cancel()


  def wait(self):
    if hasattr(self, "_timer") and self._timer:
      self._timer.join()
    self._hyper.wait()
    return self._exit_status


print
print "<VMRunner>", "Validating test"

validate_test.load_schema("../vm.schema.json")
validate_test.has_required_stuff(".")

print
print "<VMRunner>", "Building test with 'make'"
subprocess.check_call(["make"])

default_spec = {"image" : "test.img"}

# Provide a list of VM's with validated specs
vms = []

if validate_test.valid_vms:
  print
  print "<VMRunner>", "Loaded VM specification(s) from JSON"
  for spec in validate_test.valid_vms:
    vms.append(vm(spec))

else:
  print
  print "<VMRunner>", "No VM specification JSON found, trying default: ", default_spec
  vms.append(vm(default_spec))
