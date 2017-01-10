import os
import sys
import subprocess
import thread
import threading
import time
import re
import linecache
import traceback
import validate_vm
import signal
import psutil

from prettify import color

INCLUDEOS_HOME = None

nametag = "<VMRunner>"

if "INCLUDEOS_PREFIX" not in os.environ:
    def_home = "/usr/local"
    print color.WARNING("WARNING:"), "Environment varialble INCLUDEOS_PREFIX is not set. Trying default", def_home
    if not os.path.isdir(def_home): raise Exception("Couldn't find INCLUDEOS_PREFIX")
    INCLUDEOS_HOME= def_home
else:
    INCLUDEOS_HOME = os.environ['INCLUDEOS_PREFIX']

package_path = os.path.dirname(os.path.realpath(__file__))

# The end-of-transmission character
EOT = chr(4)

# Exit codes used by this program
exit_codes = {"SUCCESS" : 0,
              "PROGRAM_FAILURE" : 1,
              "TIMEOUT" : 66,
              "VM_PANIC" : 67,
              "CALLBACK_FAILED" : 68,
              "BUILD_FAIL" : 69,
              "ABORT" : 70,
              "VM_EOT" : 71 }

def get_exit_code_name (exit_code):
    for name, code in exit_codes.iteritems():
        if code == exit_code: return name
    return "UNKNOWN ERROR"

# We want to catch the exceptions from callbacks, but still tell the test writer what went wrong
def print_exception():
    exc_type, exc_value, exc_traceback = sys.exc_info()
    traceback.print_exception(exc_type, exc_value, exc_traceback,
                              limit=10, file=sys.stdout)


devnull = open(os.devnull, 'w')

# Check for prompt-free sudo access
def have_sudo():
    try:
        subprocess.check_output(["sudo", "-n", "whoami"], stderr = devnull) == 0
    except Exception as e:
        raise Exception("Sudo access required")

    return True

# Run a command, pretty print output, throw on error
def cmd(cmdlist):
    res = subprocess.check_output(cmdlist)
    for line in res.rstrip().split("\n"):
        print color.SUBPROC(line)

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


# Qemu Hypervisor interface
class qemu(hypervisor):

    def __init__(self, config):
        super(qemu, self).__init__(config)
        self._proc = None
        self._stopped = False
        self._nametag = "<" + type(self).__name__ + ">"
        self._sudo = False

    def name(self):
        return "Qemu"

    def drive_arg(self, filename, drive_type="virtio", drive_format="raw", media_type="disk"):
        return ["-drive","file="+filename+",format="+drive_format+",if="+drive_type+",media="+media_type]

    def net_arg(self, backend = "tap", device = "virtio", if_name = "net0", mac="c0:01:0a:00:00:2a"):
        device_names = {"virtio" : "virtio-net"}
        qemu_ifup = INCLUDEOS_HOME+"/includeos/scripts/qemu-ifup"
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

    # Start a process and preserve in- and output pipes
    # Note: if the command failed, we can't know until we have exit status,
    # but we can't wait since we expect no exit. Checking for program start error
    # is therefore deferred to the callee
    def start_process(self, cmdlist):

        if cmdlist[0] == "sudo": # and have_sudo():
            print color.WARNING("Running with sudo")
            self._sudo = True

        # Start a subprocess
        self._proc = subprocess.Popen(cmdlist,
                                      stdout = subprocess.PIPE,
                                      stderr = subprocess.PIPE,
                                      stdin = subprocess.PIPE)
        print color.INFO(self._nametag), "Started process PID ",self._proc.pid

        return self._proc


    def get_error_messages(self):
        if self._proc.poll():
            data, err = self._proc.communicate()
            return err

    def boot(self, multiboot, kernel_args):
        self._stopped = False
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

        # TODO: sudo is only required for tap networking and kvm. Check for those.
        command = ["sudo", "qemu-system-x86_64"]
        if self.kvm_present(): command.append("--enable-kvm")

        command += kernel_args

        command += ["-nographic" ] + disk_args + net_args + mem_arg

        print color.INFO(self._nametag), "command:"
        print color.DATA(" ".join(command))

        try:
            self.start_process(command)
        except Exception as e:
            print color.INFO(self._nametag),"Starting subprocess threw exception:", e
            raise e

    def stop(self):

        signal = "-SIGTERM"

        # Don't try to kill twice
        if self._stopped:
            self.wait()
            return self
        else:
            self._stopped = True

        if self._proc and self._proc.poll() == None :

            if not self._sudo:
                print color.INFO(self._nametag),"Stopping child process (no sudo required)"
                self._proc.terminate()
            else:
                # Find and terminate all child processes, since parent is "sudo"
                parent = psutil.Process(self._proc.pid)
                children = parent.children()

                print color.INFO(self._nametag), "Stopping", self._config["image"], "PID",self._proc.pid, "with", signal

                for child in children:
                    print color.INFO(self._nametag)," + child process ", child.pid

                    # The process might have gotten an exit status by now so check again to avoid negative exit
                    if (not self._proc.poll()):
                        subprocess.call(["sudo", "kill", signal, str(child.pid)])

            # Wait for termination (avoids the need to reset the terminal etc.)
            self.wait()

        return self

    def wait(self):
        if (self._proc): self._proc.wait()
        return self

    def read_until_EOT(self):
        chars = ""

        while (not self._proc.poll()):
            char = self._proc.stdout.read(1)
            if char == chr(4):
                return chars
            chars += char

        return chars


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
        self._exit_msg = ""
        self._config = config
        self._on_success = lambda(line) : self.exit(exit_codes["SUCCESS"], nametag + " All tests passed")
        self._on_panic =  self.panic
        self._on_timeout = self.timeout
        self._on_output = {
            "PANIC" : self._on_panic,
            "SUCCESS" : self._on_success }
        assert(issubclass(hyper, hypervisor))
        self._hyper  = hyper(config)
        self._timeout_after = None
        self._timer = None
        self._on_exit_success = lambda : None
        self._on_exit = lambda : None
        self._root = os.getcwd()

    def stop(self):
        self._hyper.stop().wait()
        if self._timer:
            self._timer.cancel()
        return self

    def wait(self):
        if hasattr(self, "_timer") and self._timer:
            self._timer.join()
        self._hyper.wait()
        return self._exit_status

    def poll(self):
        return self._hyper.poll()

    def exit(self, status, msg):
        self._exit_status = status
        self.stop()
        print color.INFO(nametag),"Exit called with status", self._exit_status, "(",get_exit_code_name(self._exit_status),")"
        print color.INFO(nametag),"Calling on_exit"
        # Change back to test source
        os.chdir(self._root)
        self._on_exit()
        if status == 0:
            # Print success message and return to caller
            print color.SUCCESS(msg)
            print color.INFO(nametag),"Calling on_exit_success"
            return self._on_exit_success()

        # Print fail message and exit with appropriate code
        print color.EXIT_ERROR(get_exit_code_name(status), msg)
        sys.exit(status)

    # Default timeout event
    def timeout(self):
        print color.INFO("<timeout>"), "VM timed out"

        # Note: we have to stop the VM since the main thread is blocking on vm.readline
        #self.exit(exit_codes["TIMEOUT"], nametag + " Test timed out")
        self._exit_status = exit_codes["TIMEOUT"]
        self._exit_msg = "vmrunner timed out after " + str(self._timeout_after) + " seconds"
        self._hyper.stop().wait()

    # Default panic event
    def panic(self, panic_line):
        panic_reason = self._hyper.readline()
        print color.INFO(nametag), "VM signalled PANIC. Reading until EOT (", hex(ord(EOT)), ")"
        print color.VM(panic_reason),
        remaining_output = self._hyper.read_until_EOT()
        for line in remaining_output.split("\n"):
            print color.VM(line)

        self.exit(exit_codes["VM_PANIC"], panic_reason)


    # Events - subscribable
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

    # Read a line from the VM's standard out
    def readline(self):
        return self._hyper.readline()

    # Write a line to VM stdout
    def writeline(self, line):
        return self._hyper.writeline(line)

    # Make using GNU Make
    def make(self, params = []):
        print color.INFO(nametag), "Building with 'make' (params=" + str(params) + ")"
        make = ["make"]
        make.extend(params)
        cmd(make)
        return self

    # Call cmake
    def cmake(self, args = []):
        print color.INFO(nametag), "Building with cmake (%s)" % args
        # install dir:
        INSTDIR = os.getcwd()

        # create build directory
        try:
            os.makedirs("build")
        except OSError as err:
            if err.errno!=17: # Errno 17: File exists
                self.exit(exit_codes["BUILD_FAIL"], "could not create build directory")

        # go into build directory
        # NOTE: The test gets run from here
        os.chdir("build")

        # build with prefix = original path
        cmake = ["cmake", "..", "-DCMAKE_INSTALL_PREFIX:PATH=" + INSTDIR]
        cmake.extend(args)

        try:
            cmd(cmake)

            # if everything went well, build with make and install
            return self.make()
        except Exception as e:
            print "Excetption while building: ", e
            self.exit(exit_codes["BUILD_FAIL"], "building with cmake failed")

    # Clean cmake build folder
    def clean(self):
        print color.INFO(nametag), "Cleaning cmake build folder"
        subprocess.call(["rm","-rf","build"])

    # Boot the VM and start reading output. This is the main event loop.
    def boot(self, timeout = 60, multiboot = True, kernel_args = "booted with vmrunner"):

        # This might be a reboot
        self._exit_status = None
        self._timeout_after = timeout

        # Start the timeout thread
        if (timeout):
            print color.INFO(nametag),"setting timeout to",timeout,"seconds"
            self._timer = threading.Timer(timeout, self._on_timeout)
            self._timer.start()

        # Boot via hypervisor
        try:
            self._hyper.boot(multiboot, kernel_args)
        except Exception as err:
            print color.WARNING("Exception raised while booting ")
            if (timeout): self._timer.cancel()
            self.exit(exit_codes["CALLBACK_FAILED"], str(err))

        # Start analyzing output
        while self._hyper.poll() == None and not self._exit_status:

            try:
                line = self._hyper.readline()
            except Exception as e:
                print color.WARNING("Exception thrown while waiting for vm output")
                break

            if line:
                # Special case for end-of-transmission
                if line == EOT:
                    if not self._exit_status: self._exit_status = exit_codes["VM_EOT"]
                    break
                if line.startswith("     [ Kernel ] service exited with status"):
                    self._exit_status = int(line.split(" ")[-1].rstrip())
                    self._exit_msg = "Service exited"
                    break
                else:
                    print color.VM(line.rstrip())

            else:
                pass
                # TODO: Add event-trigger for EOF?

            for pattern, func in self._on_output.iteritems():
                if re.search(pattern, line):
                    try:
                        res = func(line)
                    except Exception as err:
                        print color.WARNING("Exception raised in event callback: ")
                        print_exception()
                        res = False
                        self.stop()

                    # NOTE: It can be 'None' without problem
                    if res == False:
                        self._exit_status = exit_codes["CALLBACK_FAILED"]
                        self.exit(self._exit_status, " Event-triggered test failed")

        # If the VM process didn't exit by now we need to stop it.
        if (self.poll() == None):
            self.stop()

        # We might have an exit status, e.g. set by a callback noticing something wrong with VM output
        if self._exit_status:
            self.exit(self._exit_status, self._exit_msg)

        # Process might have ended prematurely
        elif self.poll():
            self.exit(self._hyper.poll(), self._hyper.get_error_messages())

        # If everything went well we can return
        return self


print color.HEADER("IncludeOS vmrunner loading VM configs")

schema_path = package_path + "/vm.schema.json"
print color.INFO(nametag), "Validating JSON according to schema ",schema_path
validate_vm.load_schema(schema_path)
validate_vm.has_required_stuff(".")

default_spec = {"image" : "test.img"}

# Provide a list of VM's with validated specs
vms = []

if validate_vm.valid_vms:
    print color.INFO(nametag), "Loaded VM specification(s) from JSON"
    for spec in validate_vm.valid_vms:
        print color.INFO(nametag), "Found VM spec: "
        print color.DATA(spec.__str__())
        vms.append(vm(spec))

else:
    print color.WARNING(nametag), "No VM specification JSON found, trying default: ", default_spec
    vms.append(vm(default_spec))



# Handler for SIGINT
def handler(signum, frame):
    print
    print color.WARNING("Process interrupted - stopping vms")
    for vm in vms:
        try:
            vm.exit(exit_codes["ABORT"], "Process terminated by user")
        except Exception as e:
            print color.WARNING("Forced shutdown caused exception: "), e
            raise e


signal.signal(signal.SIGINT, handler)
