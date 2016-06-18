print "vmrunner.py running"

import os
import sys
import subprocess
import threading
import time
print os.getcwd()
import re

import validate_test

INCLUDEOS_HOME = None

if not os.environ.has_key("INCLUDEOS_HOME"):
    print "WARNING: Environment varialble INCLUDEOS_HOME is not set. Trying default"
    def_home = os.environ["HOME"] + "/IncludeOS_install"
    if not os.path.isdir(def_home): raise Exception("Couldn't find INCLUDEOS_HOME")
    INCLUDEOS_HOME= def_home
else:
    INCLUDEOS_HOME = os.environ['INCLUDEOS_HOME']

# Exit codes used by this program
exit_codes = {"SUCCESS" : 0,
              "PROGRAM_FAILURE" : 1,
              "TIMEOUT" : 66,
              "VM_FAIL" : 67,
              "OUTSIDE_FAIL" : 68 }


def abstract():
    raise Exception("Abstract class method called. Use a subclass")
# Hypervisor base / super class
# (It seems to be recommended for "new style classes" to inherit object)
class hypervisor(object):

  def __init__(self, config):
    self._config = config;

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
    qemu_ifup = INCLUDEOS_HOME+"/etc/qemu-ifup"
    return ["-device", type_names[if_type]+",netdev="+if_name+",mac="+mac,
            "-netdev", "tap,id="+if_name+",script="+qemu_ifup]

  def kvm_present(self):
    command = "egrep -m 1 '^flags.*(vmx|svm)' /proc/cpuinfo"
    try:
      subprocess.check_output(command, shell = True)
      print "<qemu> KVM ON"
      return True

    except Exception as err:
      print "<qemu> KVM OFF"
      return False

  def boot(self):
    self._out_sign = "<" + type(self).__name__ + ">"
    print self._out_sign,"booting ",self._config["image"]

    disk_args = self.drive_arg(self._config["image"], "ide")
    if self._config.has_key("drives"):
      for disk in self._config["drives"]:
        disk_args += self.drive_arg(disk["file"], disk["type"], disk["format"])

    net_args = []
    i = 0
    if self._config.has_key("net"):
      for net in self._config["net"]:
        net_args += self.net_arg(net["type"], "net"+str(i), net["mac"])
        i+=1

    mem_arg = []
    if self._config.has_key("mem"):
      mem_arg = ["-m",str(self._config["mem"])]

    command = ["sudo", "qemu-system-x86_64"]
    if self.kvm_present(): command.append("--enable-kvm")

    command += ["-nographic" ] + disk_args + net_args + mem_arg

    print self._out_sign, "command:", command

    self._proc = start_process(command)

  def stop(self):
    if hasattr(self, "_proc") and self._proc.poll() == None :
      print self._out_sign,"Stopping", self._config["image"], "PID",self._proc.pid
      # Kill with sudo
      subprocess.check_call(["sudo","kill", "-SIGTERM", str(self._proc.pid)])
      # Wait for termination (avoids the need to reset the terminal)
      self._proc.wait()
    return self

  def wait(self):
    print self._out_sign, "Waiting for process to terminate"
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
    self._on_success = lambda : self.exit(exit_codes["SUCCESS"], "<VMRun> SUCCESS : All tests passed")
    self._on_panic =  lambda : self.exit(exit_codes["VM_FAIL"], "<VMRun> FAIL : " + self._hyper.readline())
    self._on_timeout = lambda : self.exit(exit_codes["TIMEOUT"], "<VMRun> TIMEOUT: Test timed out")
    self._on_output = {
      "PANIC" : self._on_panic,
      "SUCCESS" : self._on_success }
    assert(issubclass(hyper, hypervisor))
    self._hyper  = hyper(config)
    self._timer = None

  def exit(self, status, msg):
    self.stop()
    print
    print msg
    self._exit_status = status
    # sys.exit won't really do anything if called from a (timer) thread
    sys.exit(status)

  def on_output(self, output, callback):
    self._on_output[ output ] = callback

  def on_success(self, callback):
    self._on_success = callback

  def on_panic(self, callback):
    self._on_panic = callback

  def on_timeout(self, callback):
    self._on_timeout = callback

  def readline(self):
    return self._hyper.readline()

  def boot(self, timeout = None):

    # Start the timeout thread
    if (timeout):
      self._timer = threading.Timer(timeout, self._on_timeout)
      self._timer.start()

    # Boot via hypervisor
    try:
      self._hyper.boot()
    except Exception as err:
      if (timeout): self._timer.cancel()
      raise err
      #self.exit(1, err)

    # Start analyzing output
    while self._hyper.poll() == None and not self._exit_status:
      line = self._hyper.readline()
      print "<VM>",line.rstrip()
      # Look for event-triggers
      for pattern, func in self._on_output.iteritems():
        if re.search(pattern, line):
          try:
            res = func()
          except Exception as err:
            print "Error in user callback: ",err
            res = False
          #NOTE: It can be 'None' without problem
          if res == False:
            self._exit_status = exit_codes["OUTSIDE_FAIL"]
            self.exit(self._exit_status, "<VMRun> External test failed")
            print "<VMRunner> VM-external test failed"



    # Now we either have an exit status from timer thread, or an exit status
    # from the subprocess.
    self.stop()
    self.wait()

    if self._exit_status:
        print "<VMRunner> Done running VM. Exit status: ", self._exit_status
        sys.exit(self._exit_status)
    else:
        print "<VMRunner> Subprocess finished. Exiting with ", self._hyper.poll()
        sys.exit(self._hyper.poll())

    raise Exception("Unexpected termination")

  def stop(self):
    self._hyper.stop()
    if hasattr(self, "_timer") and self._timer:
      self._timer.cancel()
    return self

  def wait(self):
    if hasattr(self, "_timer") and self._timer:
      self._timer.join()
    self._hyper.wait()
    return self._exit_status


  def poll(self):
    return self._hyper.poll()


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
    print "<VMRunner> Found VM spec: ", spec
    vms.append(vm(spec))

else:
  print
  print "<VMRunner>", "No VM specification JSON found, trying default: ", default_spec
  vms.append(vm(default_spec))
