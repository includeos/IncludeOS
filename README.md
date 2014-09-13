IncludeOS 
================================================

IncludeOS is an includable operating system for C++ services running in cloud. By "includable" we mean that `#include <os>` will literally include a whole little operating system ABI into your service. The build system will then link your service and the OS objects into a single binary, attach a boot loader and combine all that into a self-contained bootable disk image, ready to run on a modern hypervisor. 


## Prerequisites 
  * A machine with at least 1024 MB memory. (At least for ubuntu I ran out during compilation of toolchain with 512 MB)
  * Ubuntu 14.04 LTS, Vanilla (I use lubuntu, to support VM graphics if necessary)
  * Git

(I'm using an Ubuntu VM, running virtualbox inside a mac.)

## Installation

Once you have a system with the prereqs (virtual or not), everything should be set up by:

    $sudo apt-get install git
    $git clone https://github.com/hioa-cs/IncludeOS
    $cd IncludeOS
    $sudo ./install.sh

### The script is supposed to...:
* Build a cross compiler according to [osdev howto](http://wiki.osdev.org/GCC_Cross-Compiler) - I do it exactly like that, except that I use `/usr/local/cross/` as path, instead of `/opt/cross`. 
* Build [Redhat's newlib](https://sourceware.org/newlib/), using the cross compiler, and install it according to `./etc/build_newlib.sh`. The script will also install it, to the `exported` location.
* Build and install the IncludeOS library, which your service will be linked with


NOTE: 
* If you want to debug the bootloader, or inspect memory, registers, flags etc. using a GUI, you need to install [bochs](http://bochs.sourceforge.net/). See `./etc/bochs_installation.sh` for build options, and `./etc/.bochsrc` for an example config. file, (which specifies a <1MB disk).


## Testing the installation

A successful setup should enable you to build and run a virtual machine. Some code for a very simple one is provided. The command

    $./test.sh 

will build and run a VM for you, and let you know if everything worked out. 

## VirtualBox config
  * VirtualBox does not support nested virtualization (a [ticket](https://www.virtualbox.org/ticket/4032) has been open for 5 years). This means you can't use the kvm module, but you can use Qemu directly. It will be slower, but a small VM still boots in no time. For this reason, this install script does not require kvm or nested virtualization.
  * You might want to install Virtual box vbox additions, if want screen scaling. The above provides the requisites for this (compiler stuff). 


## Now what?
Once you've run a successful test (i.e. you got some boot messages from Qemu, a simple hello from the demo service, followed by `>>> System idle - everything seems OK` ) you know IncludeOS works on your machine, and you can go ahead and tinker. 

### Start tinkering
Feel free! A few things to note:

* The user is supposed to start implementation by copying the [./seed](./seed) directory to a convenient location like `~/your_service`. You can then start implementing the `start` function in the `service` class, located in [your_service/service.cpp](./seed/service.cpp) (Very simple example provided). This function will be called once the OS is up and running. 
* The whole boot sequence consists of the following steps:
  1. BIOS loads [bootloader.asm](./src/bootloader.asm), starting at `_start`. 
  2. The bootloader sets up segments, switches to protected mode, loads the service (a binary `service` consisting of the OS classes and the service) from disk.
  3. The bootloader hands over control to the kernel, which starts at the `_start` symbol inside [kernel_boot.cpp](kernel_boot.cpp). 
  4. The kernel initializes `.bss`, calls clobal constructors (`_init`), and then calls `main` which just calls `OS::start` in [class_os.cpp](./src/class_os.cpp), which again (is supposed to) set up interrupts, initialize devices +++, etc. etc.
  5. Finally the OS class (still `OS::start`) calls `Service::start()`, handing over control to the user.
* The build sequence consists of the following steps:
  1. Assemble [bootloader.asm](./src/bootloader.asm), into a boot sector `bootloader`.
  2. Compile the service and everything it needs, except pre-compiled libraries (such as newlib), into object files (.o)
  3. Statically link all the parts together into one elf-binary, `your_service`.
  4. Use `./vmbuild` (Which was also compiled if needed) to combine the `bootloader` and `your_service` into a disk image called `your_service.img`. At this point the bootloader gets the size- and location of the service hardcoded into it.
  5. Run qemu with the image as hard disk.
* Inspect the [Makefile](./src/Makefile) and [linker script, linker.ld](./src/linker.ld) for more information about how the build happens, and [vmbuild/vmbuild.cpp](./vmbuild/vmbuild.cpp) for how the image gets constructed.

### Helper scripts
There's a convenience script, [./seed/run.sh](./seed/run.sh), which has the "Make-vmbuild-qemu" sequence laid out, with special options for debugging (It will add debugging symbols to the elf-binary and start qemu in debugging mode, ready for connection with `gdb`. More on this inside the script.). I use this script to run the code, where I'd normally just run the program from a shell. Don't worry, it's fast, even in nested/emulated mode.
