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
from shutil import copyfile

from prettify import color

INCLUDEOS_HOME = None

if "INCLUDEOS_PREFIX" not in os.environ:
    def_home = "/usr/local"
    print color.WARNING("WARNING:"), "Environment variable INCLUDEOS_PREFIX is not set. Trying default", def_home
    if not os.path.isdir(def_home): raise Exception("Couldn't find INCLUDEOS_PREFIX")
    INCLUDEOS_HOME= def_home
else:
    INCLUDEOS_HOME = os.environ['INCLUDEOS_PREFIX']

package_path = os.path.dirname(os.path.realpath(__file__))

default_config = INCLUDEOS_HOME + "/tools/vmrunner/vm.default.json"

default_json = "./vm.json"

chainloader = INCLUDEOS_HOME + "/bin/chainloader"

# Provide a list of VM's with validated specs
# (One default vm added at the end)
vms = []

panic_signature = "\\x15\\x07\\t\*\*\*\* PANIC \*\*\*\*"
nametag = "<VMRunner>"
INFO = color.INFO(nametag)
VERB = bool(os.environ["VERBOSE"]) if "VERBOSE" in os.environ else False

class Logger:
    def __init__(self, tag):
        self.tag = tag
        if (VERB):
            self.info = self.info_verb
        else:
            self.info = self.info_silent

    def __call__(self, *args):
        self.info(args)

    def info_verb(self, args):
        print self.tag,
        for arg in args:
            print arg,
        print

    def info_silent(self, args):
        pass

# Define verbose printing function "info", with multiple args
default_logger = Logger(INFO)
def info(*args):
    default_logger.info(args)



# Example on Ubuntu:
# ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), statically linked, not stripped
# ELF 64-bit LSB executable, x86-64, version 1 (SYSV), statically linked, not stripped
#
# Example on mac (same files):
# ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), statically linked, not stripped, with debug_info
# ELF 64-bit LSB executable, x86-64, version 1 (SYSV), statically linked, not stripped, with debug_info
#
# Mac native:
# Mach-O 64-bit x86_64 executable, flags:<NOUNDEFS|DYLDLINK|TWOLEVEL|WEAK_DEFINES|BINDS_TO_WEAK|PIE>



def file_type(filename):
    p = subprocess.Popen(['file',filename],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    output, errors = p.communicate()
    return output

def is_Elf64(filename):
    magic = file_type(filename)
    return "ELF" in magic and "executable" in magic and "64-bit" in magic

def is_Elf32(filename):
    magic = file_type(filename)
    return "ELF" in magic and "executable" in magic and "32-bit" in magic


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
              "VM_EOT" : 71,
              "BOOT_FAILED": 72,
              "PARSE_ERROR": 73
}

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

    def image_name(self):
        abstract()

    def start_process(self, cmdlist):

        if cmdlist[0] == "sudo": # and have_sudo():
            print color.WARNING("Running with sudo")
            self._sudo = True

        # Start a subprocess
        self._proc = subprocess.Popen(cmdlist,
                                      stdout = subprocess.PIPE,
                                      stderr = subprocess.PIPE,
                                      stdin = subprocess.PIPE)

        return self._proc


# Solo5-hvt Hypervisor interface
class solo5(hypervisor):

    def __init__(self, config):
        # config is not yet used for solo5
        super(solo5, self).__init__(config)
        self._proc = None
        self._stopped = False
        self._sudo = False
        self._image_name = self._config if "image" in self._config else self.name() + " vm"

        # Pretty printing
        self.info = Logger(color.INFO("<" + type(self).__name__ + ">"))

    def name(self):
        return "Solo5-hvt"

    def image_name(self):
        return self._image_name

    def drive_arg(self, filename,
                  device_format="raw", media_type="disk"):
        if device_format != "raw":
            raise Exception("solo5 can only handle drives in raw format.")
        if media_type != "disk":
            raise Exception("solo5 can only handle drives of type disk.")
        return ["--disk=" + filename]

    def net_arg(self):
        return ["--net=tap100"]

    def get_final_output(self):
        return self._proc.communicate()

    def boot(self, multiboot, debug=False, kernel_args = "", image_name = None):
        self._stopped = False

        qkvm_bin = INCLUDEOS_HOME + "/x86_64/lib/solo5-hvt"

        # Use provided image name if set, otherwise raise an execption
        if not image_name:
            raise Exception("No image name provided as param")

        self._image_name = image_name

        command = ["sudo", qkvm_bin]

        if not "drives" in self._config:
            command += self.drive_arg(self._image_name)
        elif len(self._config["drives"]) > 1:
            raise Exception("solo5/solo5 can only handle one drive.")
        else:
            for disk in self._config["drives"]:
                info ("Ignoring drive type argument: ", disk["type"])
                command += self.drive_arg(disk["file"], disk["format"],
                                          disk["media"])

        command += self.net_arg()
        command += [self._image_name]
        command += [kernel_args]

        try:
            self.info("Starting ", command)
            self.start_process(command)
            self.info("Started process PID ",self._proc.pid)
        except Exception as e:
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
                info ("Stopping child process (no sudo required)")
                self._proc.terminate()
            else:
                # Find and terminate all child processes, since parent is "sudo"
                parent = psutil.Process(self._proc.pid)
                children = parent.children()

                info ("Stopping", self._image_name, "PID",self._proc.pid, "with", signal)

                for child in children:
                    info (" + child process ", child.pid)

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

# Qemu Hypervisor interface
class qemu(hypervisor):

    def __init__(self, config):
        super(qemu, self).__init__(config)
        self._proc = None
        self._stopped = False
        self._sudo = False
        self._image_name = self._config if "image" in self._config else self.name() + " vm"
        self.m_drive_no = 0

        # Pretty printing
        self.info = Logger(color.INFO("<" + type(self).__name__ + ">"))

    def name(self):
        return "Qemu"

    def image_name(self):
        return self._image_name

    def drive_arg(self, filename, device = "virtio", drive_format = "raw", media_type = "disk"):
        names = {"virtio" : "virtio-blk",
                 "virtio-scsi" : "virtio-scsi",
                 "ide"    : "piix3-ide",
                 "nvme"   : "nvme"}

        if device == "ide":
            # most likely a problem relating to bus, or wrong .drive
            return ["-drive","file=" + filename
                    + ",format=" + drive_format
                    + ",if=" + device
                    + ",media=" + media_type]
        else:
            if device in names:
                device = names[device]

            driveno = "drv" + str(self.m_drive_no)
            self.m_drive_no += 1
            return ["-drive", "file=" + filename + ",format=" + drive_format
                            + ",if=none" + ",media=" + media_type + ",id=" + driveno,
                    "-device",  device + ",drive=" + driveno +",serial=foo"]

    # -initrd "file1 arg=foo,file2"
    # This syntax is only available with multiboot.

    def mod_args(self, mods):
        mods_list =",".join([mod["path"] + ((" " + mod["args"]) if "args" in mod else "")
                             for mod in mods])
        return ["-initrd", mods_list]

    def net_arg(self, backend, device, if_name = "net0", mac = None, bridge = None, scripts = None):
        if scripts:
            qemu_ifup = scripts + "qemu-ifup"
            qemu_ifdown = scripts + "qemu-ifdown"
        else:
            qemu_ifup = INCLUDEOS_HOME + "/scripts/qemu-ifup"
            qemu_ifdown = INCLUDEOS_HOME + "/scripts/qemu-ifdown"

        # FIXME: this needs to get removed, e.g. fetched from the schema
        names = {"virtio" : "virtio-net", "vmxnet" : "vmxnet3", "vmxnet3" : "vmxnet3"}

        if device in names:
            device = names[device]

        # Network device - e.g. host side of nic
        netdev = backend + ",id=" + if_name

        if backend == "tap":
            if self._kvm_present:
                netdev += ",vhost=on"
            netdev += ",script=" + qemu_ifup + ",downscript=" + qemu_ifdown

        if bridge:
            netdev = "bridge,id=" + if_name + ",br=" + bridge

        # Device - e.g. guest side of nic
        device += ",netdev=" + if_name

        # Add mac-address if specified
        if mac: device += ",mac=" + mac
        device += ",romfile=" # remove some qemu boot info (experimental)

        return ["-device", device,
                "-netdev", netdev]

    def kvm_present(self):
        command = "egrep -m 1 '^flags.*(vmx|svm)' /proc/cpuinfo"
        try:
            subprocess.check_output(command, shell = True)
            self.info("KVM ON")
            return True

        except Exception as err:
            self.info("KVM OFF")
            return False

    # Start a process and preserve in- and output pipes
    # Note: if the command failed, we can't know until we have exit status,
    # but we can't wait since we expect no exit. Checking for program start error
    # is therefore deferred to the callee

    def get_final_output(self):
        return self._proc.communicate()

    def boot(self, multiboot, debug = False, kernel_args = "", image_name = None):
        self._stopped = False

        info ("Booting with multiboot:", multiboot, "kernel_args: ", kernel_args, "image_name:", image_name)

        # Resolve if kvm is present
        self._kvm_present = self.kvm_present()

        # Use provided image name if set, otherwise try to find it in json-config
        if not image_name:
            if not "image" in self._config:
                raise Exception("No image name provided, neither as param or in config file")
            image_name = self._config["image"]

        self._image_name = image_name

        disk_args = []

        debug_args = []
        if debug:
            debug_args = ["-s"]

        # multiboot - e.g. boot with '-kernel' and no bootloader
        if multiboot:

            # TODO: Remove .img-extension from vm.json in tests to avoid this hack
            if (image_name.endswith(".img")):
                image_name = image_name.split(".")[0]

            if not kernel_args: kernel_args = "\"\""

            info ("File magic: ", file_type(image_name))

            if is_Elf64(image_name):
                info ("Found 64-bit ELF, need chainloader" )
                print "Looking for chainloader: "
                print "Found", chainloader, "Type: ",  file_type(chainloader)
                if not is_Elf32(chainloader):
                    print color.WARNING("Chainloader doesn't seem to be a 32-bit ELF executable")
                kernel_args = ["-kernel", chainloader, "-append", kernel_args, "-initrd", image_name + " " + kernel_args]
            elif is_Elf32(image_name):
                info ("Found 32-bit elf, trying direct boot")
                kernel_args = ["-kernel", image_name, "-append", kernel_args]
            else:
                print color.WARNING("Provided kernel is neither 64-bit or 32-bit ELF executable.")
                kernel_args = ["-kernel", image_name, "-append", kernel_args]

            info ( "Booting", image_name, "directly without bootloader (multiboot / -kernel args)")
        else:
            kernel_args = []
            image_in_config = False

            # If the provided image name is also defined in vm.json, use vm.json
            if "drives" in self._config:
                for disk in self._config["drives"]:
                    if disk["file"] == image_name:
                        image_in_config = True
                if not image_in_config:
                    info ("Provided image", image_name, "not found in config. Appending.")
                    self._config["drives"].insert(0, {"file" : image_name, "type":"ide", "format":"raw", "media":"disk"})
            else:
                self._config["drives"] =[{"file" : image_name, "type":"ide", "format":"raw", "media":"disk"}]

            info ("Booting", image_name, "with a bootable disk image")

        if "drives" in self._config:
            for disk in self._config["drives"]:
                disk_args += self.drive_arg(disk["file"], disk["type"], disk["format"], disk["media"])

        mod_args = []
        if "modules" in self._config:
            mod_args += self.mod_args(self._config["modules"])

        if "bios" in self._config:
            kernel_args.extend(["-bios", self._config["bios"]])

        if "uuid" in self._config:
            kernel_args.extend(["--uuid", str(self._config["uuid"])])

        if "smp" in self._config:
            kernel_args.extend(["-smp", str(self._config["smp"])])

        if "cpu" in self._config:
            cpu = self._config["cpu"]
            cpu_str = cpu["model"]
            if "features" in cpu:
                cpu_str += ",+" + ",+".join(cpu["features"])
            kernel_args.extend(["-cpu", cpu_str])

        net_args = []
        i = 0
        if "net" in self._config:
            for net in self._config["net"]:
                mac = net["mac"] if "mac" in net else None
                bridge = net["bridge"] if "bridge" in net else None
                scripts = net["scripts"] if "scripts" in net else None
                net_args += self.net_arg(net["backend"], net["device"], "net"+str(i), mac, bridge, scripts)
                i+=1

        mem_arg = []
        if "mem" in self._config:
            mem_arg = ["-m", str(self._config["mem"])]

        vga_arg = ["-nographic" ]
        if "vga" in self._config:
            vga_arg = ["-vga", str(self._config["vga"])]

        trace_arg = []
        if "trace" in self._config:
            trace_arg = ["-trace", "events=" + str(self._config["trace"])]

        pci_arg = []
        if "vfio" in self._config:
            pci_arg = ["-device", "vfio-pci,host=" + self._config["vfio"]]

        # custom qemu binary/location
        qemu_binary = "qemu-system-x86_64"
        if "qemu" in self._config:
            qemu_binary = self._config["qemu"]

        # TODO: sudo is only required for tap networking and kvm. Check for those.
        command = ["sudo", qemu_binary]
        if self._kvm_present: command.extend(["--enable-kvm"])

        command += kernel_args
        command += disk_args + debug_args + net_args + mem_arg + mod_args
        command += vga_arg + trace_arg + pci_arg

        #command_str = " ".join(command)
        #command_str.encode('ascii','ignore')
        #command = command_str.split(" ")

        info("Command:", " ".join(command))

        try:
            self.start_process(command)
            self.info("Started process PID ",self._proc.pid)
        except Exception as e:
            print self.INFO,"Starting subprocess threw exception:", e
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
                info ("Stopping child process (no sudo required)")
                self._proc.terminate()
            else:
                # Find and terminate all child processes, since parent is "sudo"
                parent = psutil.Process(self._proc.pid)
                children = parent.children()

                info ("Stopping", self._image_name, "PID",self._proc.pid, "with", signal)

                for child in children:
                    info (" + child process ", child.pid)

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

    def __init__(self, config = None, hyper_name = "qemu"):

        self._exit_status = None
        self._exit_msg = ""
        self._exit_complete = False

        self._config = load_with_default_config(True, config)
        self._on_success = lambda(line) : self.exit(exit_codes["SUCCESS"], nametag + " All tests passed")
        self._on_panic =  self.panic
        self._on_timeout = self.timeout
        self._on_output = {
            panic_signature : self._on_panic,
            "SUCCESS" : self._on_success }

        if hyper_name == "solo5":
            hyper = solo5
        else:
            hyper = qemu

        # Initialize hypervisor with config
        assert(issubclass(hyper, hypervisor))
        self._hyper  = hyper(self._config)
        self._timeout_after = None
        self._timer = None
        self._on_exit_success = lambda : None
        self._on_exit = lambda : None
        self._root = os.getcwd()
        self._kvm_present = False

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

    # Stop the VM with exit status / msg.
    # set keep_running to indicate that the program should continue
    def exit(self, status, msg, keep_running = False):

        # Exit may have been called allready
        if self._exit_complete:
            return

        self._exit_status = status
        self._exit_msg = msg
        self.stop()

        # Change back to test source
        os.chdir(self._root)


        info("Exit called with status", self._exit_status, "(",get_exit_code_name(self._exit_status),")")
        info("Message:", msg, "Keep running: ", keep_running)

        if keep_running:
            return

        if self._on_exit:
            info("Calling on_exit")
            self._on_exit()


        if status == 0:
            if self._on_exit_success:
                info("Calling on_exit_success")
                self._on_exit_success()

            print color.SUCCESS(msg)
            self._exit_complete = True
            return

        self._exit_complete = True
        program_exit(status, msg)

    # Default timeout event
    def timeout(self):
        if VERB: print color.INFO("<timeout>"), "VM timed out"

        # Note: we have to stop the VM since the main thread is blocking on vm.readline
        self._exit_status = exit_codes["TIMEOUT"]
        self._exit_msg = "vmrunner timed out after " + str(self._timeout_after) + " seconds"
        self._hyper.stop().wait()

    # Default panic event
    def panic(self, panic_line):
        panic_reason = self._hyper.readline()
        info("VM signalled PANIC. Reading until EOT (", hex(ord(EOT)), ")")
        print color.VM(panic_reason),
        remaining_output = self._hyper.read_until_EOT()
        for line in remaining_output.split("\n"):
            print color.VM(line)

        self.exit(exit_codes["VM_PANIC"], panic_reason)


    # Events - subscribable
    def on_output(self, output, callback):
        self._on_output[ output ] = callback
        return self

    def on_success(self, callback, do_exit = True):
        if do_exit:
            self._on_output["SUCCESS"] = lambda(line) : [callback(line), self._on_success(line)]
        else: self._on_output["SUCCESS"] = callback
        return self

    def on_panic(self, callback, do_exit = True):
        if do_exit:
            self._on_output[panic_signature] = lambda(line) : [callback(line), self._on_panic(line)]
        else: self._on_output[panic_signature] = callback
        return self

    def on_timeout(self, callback):
        self._on_timeout = callback
        return self

    def on_exit_success(self, callback):
        self._on_exit_success = callback
        return self

    def on_exit(self, callback):
        self._on_exit = callback
        return self

    # Read a line from the VM's standard out
    def readline(self):
        return self._hyper.readline()

    # Write a line to VM stdout
    def writeline(self, line):
        return self._hyper.writeline(line)

    # Make using GNU Make
    def make(self, params = []):
        print INFO, "Building with 'make' (params=" + str(params) + ")"
        jobs = os.environ["num_jobs"].split(" ") if "num_jobs" in os.environ else ["-j4"]
        make = ["make"] + jobs
        make.extend(params)
        cmd(make)
        return self

    # Call cmake
    def cmake(self, args = []):
        print INFO, "Building with cmake (%s)" % args
        # install dir:
        INSTDIR = os.getcwd()

        if (not os.path.isfile("CMakeLists.txt") and os.path.isfile("service.cpp")):
            # No makefile present. Copy the one from seed, inform user and pray.
            # copyfile will throw errors if it encounters any.
            copyfile(INCLUDEOS_HOME + "/seed/service/CMakeLists.txt", "CMakeLists.txt")
            print INFO, "No CMakeList.txt present. File copied from seed. Please adapt to your needs."

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
            print "Exception while building: ", e
            self.exit(exit_codes["BUILD_FAIL"], "building with cmake failed")

    # Clean cmake build folder
    def clean(self):
        print INFO, "Cleaning cmake build folder"
        subprocess.call(["rm","-rf","build"])
        return self

    def find_exit_status(self, line):

        # Kernel reports service exit status
        if (line.startswith("     [ Kernel ] service exited with status") or
            line.startswith("     [ main ] returned with status")):

            self._exit_status = int(line.split(" ")[-1].rstrip())
            self._exit_msg = "Service exited with status " + str(self._exit_status)
            return self._exit_status

        # Special case for end-of-transmission, e.g. on panic
        if line == EOT:
            self._exit_status = exit_codes["VM_EOT"]
            return self._exit_status

        return None


    def trigger_event(self, line):
        # Find any callback triggered by this line
        for pattern, func in self._on_output.iteritems():
            if re.search(pattern, line):
                try:
                    # Call it
                    res = func(line)
                except Exception as err:
                    print color.WARNING("Exception raised in event callback: ")
                    print_exception()
                    res = False
                    self.stop()

                # NOTE: Result can be 'None' without problem
                if res == False:
                    self._exit_status = exit_codes["CALLBACK_FAILED"]
                    self.exit(self._exit_status, " Event-triggered test failed")


    # Boot the VM and start reading output. This is the main event loop.
    def boot(self, timeout = 60, multiboot = True, debug = False, kernel_args = "booted with vmrunner", image_name = None):
        info ("VM boot, timeout: ", timeout, "multiboot: ", multiboot, "Kernel_args: ", kernel_args, "image_name: ", image_name)
        # This might be a reboot
        self._exit_status = None
        self._exit_complete = False
        self._timeout_after = timeout

        # Start the timeout thread
        if (timeout):
            info("setting timeout to",timeout,"seconds")
            self._timer = threading.Timer(timeout, self._on_timeout)
            self._timer.start()

        # Boot via hypervisor
        try:
            self._hyper.boot(multiboot, debug, kernel_args, image_name)
        except Exception as err:
            print color.WARNING("Exception raised while booting: ")
            print_exception()
            if (timeout): self._timer.cancel()
            self.exit(exit_codes["BOOT_FAILED"], str(err))

        # Start analyzing output
        while self._exit_status == None and self.poll() == None:

            try:
                line = self._hyper.readline()
            except Exception as e:
                print color.WARNING("Exception thrown while waiting for vm output")
                break

            if line and self.find_exit_status(line) == None:
                    print color.VM(line.rstrip())
                    self.trigger_event(line)

            # Empty line - should only happen if process exited
            else: pass


        # VM Done
        info("Event loop done. Exit status:", self._exit_status, "poll:", self.poll())

        # If the VM process didn't exit by now we need to stop it.
        if (self.poll() == None):
            self.stop()

        # Process may have ended without EOT / exit message being read yet
        # possibly normal vm shutdown
        if self.poll() != None:

            info("No poll - getting final output")
            try:
                data, err = self._hyper.get_final_output()

                # Print stderr if exit status wasnt 0
                if err and self.poll() != 0:
                    print color.WARNING("Stderr: \n" + err)

                # Parse the last output from vm
                lines = data.split("\n")
                for line in lines:
                    print color.VM(line)
                    self.find_exit_status(line)
                    # Note: keep going. Might find panic after service exit

            except Exception as e:
                pass

        # We should now have an exit status, either from a callback or VM EOT / exit msg.
        if self._exit_status != None:
            info("VM has exit status. Exiting.")
            self.exit(self._exit_status, self._exit_msg)
        else:
            self.exit(self._hyper.poll(), "process exited")

        # If everything went well we can return
        return self

# Fallback to default
def load_with_default_config(use_default, path = default_json):

    # load default config
    conf = {}
    if use_default == True:
        info("Loading default config.")
        conf = load_config(default_config)

    # load user config (or fallback)
    user_conf = load_config(path)

    if user_conf:
        if use_default == False:
            # return user_conf as is
            return user_conf
        else:
            # extend (override) default config with user config
            for key, value in user_conf.iteritems():
                conf[key] = value
            info(str(conf))
            return conf
    else:
        return conf

    program_exit(73, "No config found. Try enable default config.")

# Load a vm config.
def load_config(path):

    config = {}
    description = None

    # If path is explicitly "None", try current dir
    if not path: path = default_json

    info("Trying to load config from", path)

    if os.path.isfile(path):

        try:
            # Try loading the first valid config
            config = validate_vm.load_config(path)
            info ("Successfully loaded vm config")

        except Exception as e:
            print_exception()
            info("Could not parse VM config file(s): " + path)
            program_exit(73, str(e))

    elif os.path.isdir(path):
        try:
            configs = validate_vm.load_config(path, VERB)
            info ("Found ", len(configs), "config files")
            config = configs[0]
            info ("Trying the first valid config ")
        except Exception as e:
            info("No valid config found: ", e)
            program_exit(73, "No valid config files in " + path)


    if config.has_key("description"):
        description = config["description"]
    else:
        description = str(config)

    info('"',description,'"')

    return config


def program_exit(status, msg):
    global vms

    info("Program exit called with status", status, "(",get_exit_code_name(status),")")
    info("Stopping all vms")

    for vm in vms:
        vm.stop().wait()

    # Print status message and exit with appropriate code
    if (status):
        print color.EXIT_ERROR(get_exit_code_name(status), msg)
    else:
        print color.SUCCESS(msg)

    sys.exit(status)



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


# One unconfigured vm is created by default, which will try to load a config if booted
vms.append(vm())

signal.signal(signal.SIGINT, handler)
