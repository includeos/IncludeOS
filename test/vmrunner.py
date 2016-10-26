import os
import sys
import subprocess
import thread
import threading
import time
import re
import linecache
import traceback
import validate_test
import signal

from prettify import color

INCLUDEOS_HOME = None

nametag = "<VMRunner>"

if "INCLUDEOS_HOME" not in os.environ:
    print color.WARNING("WARNING:"), "Environment varialble INCLUDEOS_HOME is not set. Trying default"
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

# We want to catch the exceptions from callbacks, but still tell the test writer what went wrong
def print_exception():
    exc_type, exc_value, exc_traceback = sys.exc_info()
    traceback.print_exception(exc_type, exc_value, exc_traceback,
                              limit=10, file=sys.stdout)

# Catch
def handler(signum, frame):
    print color.WARNING("Process interrupted")
    thread.interrupt_main()
    thread.exit()

signal.signal(signal.SIGINT, handler)



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

  # A descriptive name
  def name(self):
    abstract()


# Start a process we expect to not finish immediately (e.g. a VM)
def start_process(popen_param_list):
  # Start a subprocess
  proc = subprocess.Popen(popen_param_list,
                          stdout = subprocess.PIPE,
                          stderr = subprocess.PIPE,
                          stdin = subprocess.PIPE)

  # After half a second it should be started, otherwise throw
  time.sleep(0.5)
  if (proc.poll()):
    data, err = proc.communicate()
    raise Exception(color.C_FAILED+"Process exited. ERROR: " + err.__str__() + " " + data + color.C_ENDC);

  print color.INFO(nametag), "Started process PID ",proc.pid
  return proc


# Qemu Hypervisor interface
class qemu(hypervisor):

  def name(self):
    return "Qemu"

  def drive_arg(self, filename, drive_type="virtio", drive_format="raw", media_type="disk"):
    return ["-drive","file="+filename+",format="+drive_format+",if="+drive_type+",media="+media_type]

  def net_arg(self, backend = "tap", device = "virtio", if_name = "net0", mac="c0:01:0a:00:00:2a"):
    device_names = {"virtio" : "virtio-net"}
    qemu_ifup = INCLUDEOS_HOME+"/etc/qemu-ifup"
    return ["-device", device_names[device]+",netdev="+if_name+",mac="+mac,
            "-netdev", backend+",id="+if_name+",script="+qemu_ifup]

  def kvm_present(self):
    command = "egrep -m 1 '^flags.*(vmx|svm)' /proc/cpuinfo"
    try:
      subprocess.check_output(command, shell = True)
      print color.INFO("<qemu>"),"KVM ON"
      return True

    except Exception as err:
      print color.INFO("<qemu>"),"KVM OFF"
      return False

  def boot(self, multiboot, kernel_args):
    self._nametag = "<" + type(self).__name__ + ">"
    print color.INFO(self._nametag), "booting", self._config["image"]

    # multiboot
    if multiboot:
      print color.INFO(self._nametag), "Booting with multiboot (-kernel args)"
      kernel_args = ["-kernel", self._config["image"].split(".")[0], "-append", kernel_args]
    else:
      kernel_args = []

    disk_args = self.drive_arg(self._config["image"], "ide")
    if "drives" in self._config:
      for disk in self._config["drives"]:
        disk_args += self.drive_arg(disk["file"], disk["type"], disk["format"], disk["media"])

    net_args = []
    i = 0
    if "net" in self._config:
      for net in self._config["net"]:
        net_args += self.net_arg(net["backend"], net["device"], "net"+str(i), net["mac"])
        i+=1

    mem_arg = []
    if "mem" in self._config:
      mem_arg = ["-m",str(self._config["mem"])]

    command = ["qemu-system-x86_64"]
    if self.kvm_present(): command.append("--enable-kvm")

    command += kernel_args

    command += ["-nographic" ] + disk_args + net_args + mem_arg

    print color.INFO(self._nametag), "command:"
    print color.DATA(command.__str__())

    self._proc = start_process(command)

  def stop(self):
    if hasattr(self, "_proc") and self._proc.poll() == None :
      print color.INFO(self._nametag),"Stopping", self._config["image"], "PID",self._proc.pid
      # Kill
      subprocess.check_call(["kill", "-SIGTERM", str(self._proc.pid)])
      # Wait for termination (avoids the need to reset the terminal)
      self._proc.wait()
    return self

  def wait(self):
    print color.INFO(self._nametag), "Waiting for process to terminate"
    self._proc.wait()

  def readline(self):
    if self._proc.poll():
      raise Exception("Process completed")
    return self._proc.stdout.readline()

  def writeline(self, line):
    if self._proc.poll():
      raise Exception("Process completed")
    return self._proc.stdin.write(line + "\n")

  def poll(self):
    return self._proc.poll()

# VM class
class vm:

  def __init__(self, config, hyper = qemu):
    self._exit_status = 0
    self._config = config
    self._on_success = lambda(line) : self.exit(exit_codes["SUCCESS"], color.SUCCESS(nametag + " All tests passed"))
    self._on_panic =  lambda(line) : self.exit(exit_codes["VM_FAIL"], color.FAIL(nametag + self._hyper.readline()))
    self._on_timeout = lambda : self.exit(exit_codes["TIMEOUT"], color.FAIL(nametag + " Test timed out"))
    self._on_output = {
      "PANIC" : self._on_panic,
      "SUCCESS" : self._on_success }
    assert(issubclass(hyper, hypervisor))
    self._hyper  = hyper(config)
    self._timer = None
    self._on_exit_success = lambda : None
    self._on_exit = lambda : None

  def exit(self, status, msg):
    self.stop()
    print
    print msg
    self._exit_status = status
    if status == 0:
      self._on_exit_success()
    self._on_exit()
    sys.exit(status)

  def on_output(self, output, callback):
    self._on_output[ output ] = callback

  def on_success(self, callback):
    self._on_output["SUCCESS"] = lambda(line) : [callback(line), self._on_success(line)]

  def on_panic(self, callback):
    self._on_output["PANIC"] = lambda(line) : [callback(line), self._on_panic(line)]

  def on_timeout(self, callback):
    self._on_timeout = callback

  def on_exit_success(self, callback):
    self._on_exit_success = callback

  def on_exit(self, callback):
    self._on_exit = callback

  def readline(self):
    return self._hyper.readline()

  def writeline(self, line):
    return self._hyper.writeline(line)

  def make(self, params = []):
    print color.INFO(nametag), "Building test service with 'make' (params=" + str(params) + ")"
    make = ["make"]
    make.extend(params)
    res = subprocess.check_output(make)
    print color.SUBPROC(res)
    return self

  def boot(self, timeout = None, multiboot = True, kernel_args = "booted with vmrunner"):

    # Check for sudo access, needed for tap network devices and the KVM module
    if os.getuid() is not 0:
        print color.FAIL("Call the script with sudo access")
        sys.exit(1)

    # Start the timeout thread
    if (timeout):
      self._timer = threading.Timer(timeout, self._on_timeout)
      self._timer.start()

    # Boot via hypervisor
    try:
      self._hyper.boot(multiboot, kernel_args + "/" + self._hyper.name())
    except Exception as err:
      print color.WARNING("Exception raised while booting ")
      if (timeout): self._timer.cancel()
      raise err

    # Start analyzing output
    while self._hyper.poll() == None and not self._exit_status:
      line = self._hyper.readline()
      print color.SUBPROC("<VM> "+line.rstrip())
      # Look for event-triggers
      for pattern, func in self._on_output.iteritems():
        if re.search(pattern, line):
          try:
            res = func(line)
          except Exception as err:
            print color.WARNING("Exception raised in event callback: ")
            print_exception()
            res = False
            self.stop().wait()

          #NOTE: It can be 'None' without problem
          if res == False:
            self._exit_status = exit_codes["OUTSIDE_FAIL"]
            self.exit(self._exit_status, color.FAIL(nametag + " VM-external test failed"))

    # Now we either have an exit status from timer thread, or an exit status
    # from the subprocess, or the VM was powered off by the external test.
    # If the process didn't exit we need to stop it.
    if (self.poll() == None):
      self.stop()

    self.wait()

    if self._exit_status:
      print color.WARNING(nametag + " Found non-zero exit status but process didn't end. ")
      print color.FAIL(nametag + " Tests failed or program error")
      print color.INFO(nametag)," Done running VM. Exit status: ", self._exit_status
      sys.exit(self._exit_status)
    else:
      print color.SUCCESS(nametag + " VM exited with 0 exit status.")
      print color.INFO(nametag), " Subprocess finished. Exiting with ", self._hyper.poll()
      sys.exit(self._hyper.poll())

    raise Exception("Unexpected termination")

  def stop(self):
    # Check for sudo access, needed for qemu commands
    if os.getuid() is not 0:
        print color.FAIL("Call the script with sudo access")
        sys.exit(1)

    print color.INFO(nametag),"Stopping VM..."
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

print color.HEADER("IncludeOS vmrunner initializing tests")
print color.INFO(nametag), "Validating test service"
validate_test.load_schema(os.environ.get("INCLUDEOS_SRC", os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0]) + "/test/vm.schema.json")
validate_test.has_required_stuff(".")

default_spec = {"image" : "test.img"}

# Provide a list of VM's with validated specs
vms = []

if validate_test.valid_vms:
  print
  print color.INFO(nametag), "Loaded VM specification(s) from JSON"
  for spec in validate_test.valid_vms:
    print color.INFO(nametag), "Found VM spec: "
    print color.DATA(spec.__str__())
    vms.append(vm(spec))

else:
  print
  print color.WARNING(nametag), "No VM specification JSON found, trying default: ", default_spec
  vms.append(vm(default_spec))
