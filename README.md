IncludeOS 
================================================

IncludeOS is an includeable, minimal library operating system for C++ services running in cloud. By "includeable" we mean that your service will start by saying `#include <os>`, which will literally include a whole little operating system ABI into your service. The build system will then link your service and the OS objects into a single binary, attach a boot loader and combine all that into a self-contained bootable disk image, ready to run on a modern hypervisor. 


## Prerequisites 
  * A machine with at least 1024 MB memory. (At least for ubuntu I ran out during compilation of toolchain with 512 MB). 
  * Ubuntu 14.04 LTS, Vanilla (I use lubuntu, to support VM graphics if necessary). You should be able to develop for IncludeOS on any platform with gcc, but the build system is currently targeted for- and tested on Ubuntu.
  * Git

(I'm using an Ubuntu VM, running virtualbox inside a mac.)

## Installation

Once you have a system with the prereqs (virtual or not), everything should be set up by:

    $ sudo apt-get install git
    $ git clone https://github.com/hioa-cs/IncludeOS
    $ cd IncludeOS
    $ sudo ./install.sh

### The script is supposed to...:
* Install any tools required for building and running IncludeOS, including GCC and Qemu. 
* Build a cross compiler along the lines of [osdev howto](http://wiki.osdev.org/GCC_Cross-Compiler). The cross compiler toolchain will be installed to `/usr/local/IncludeOS/` where the OS library will end up as well.
* Build [Redhat's newlib](https://sourceware.org/newlib/) using the cross compiler, and install it according to `./etc/build_newlib.sh`. The script will also install it to the mentioned location.
* Build and install the IncludeOS library, which your service will be linked with.
* Build and install the `vmbuild` tool, which attaches a bootloader to your service and makes a bootable disk image out of it.

### Testing the installation

A successful setup should enable you to build and run a virtual machine. Some code for a very simple one is provided. The command

    $./test.sh 

will build and run a VM for you, and let you know if everything worked out. 

## Start developing

1. Copy the [./seed](./seed) directory to a convenient location like `~/your_service`. You can then start implementing the `Service::start` function in the `Service` class, located in [your_service/service.cpp](./seed/service.cpp) (Very simple example provided). This function will be called once the OS is up and running.  
2. Enter the name of your service in the first line of the [seed Makefile](./seed/Makefile). This will be the base for the name of the final disk image.

Example: 
```
     $ cp seed ~/my_service
     $ cd ~/my_service
     $ emacs service.cpp
     ... add your code
     $ ./run.sh my_service.img
```
Take a look at the [examples](./examples). These all started out as copies of the same seed.

### Helper scripts
There's a convenience script, [./seed/run.sh](./seed/run.sh), which has the "Make-vmbuild-qemu" sequence laid out, with special options for debugging (It will add debugging symbols to the elf-binary and start qemu in debugging mode, ready for connection with `gdb`. More on this inside the script.). I use this script to run the code, where I'd normally just run the program from a shell. Don't worry, it's fast, even in nested/emulated mode.


## Features
* Virtio ethernet driver with non-blocking asynchronous I/O
* Everything happens in one thread
* Delegated IRQ handling makes race conditions in userspace impossible 
* No virtual memory overhead
* (A tcp/ip stack)
* (A http server class)

### Limitations 
* No threading by design. You want more processors? Start more VM's - they're extremely lightweight.
* No support for exceptions or runtime type information (rtti). Exceptions will be added, rtti, we don't know.
* Only partial C++ standard library based on [EASTL](https://github.com/paulhodge/EASTL). (Also the [https://sourceware.org/newlib/](newlib) C standard library is included)
* No file system (we might add one, but TCP/IP comes first)
* No memory protection. If you want to overwrite the kernel, feel free, it's just a part of your own process. 

## The build & boot process

### The build process is like this:
  1. Installing IncludeOS means building a static library `os.a`, and putting it (usually) in `/usr/local/IncludeOS` along with all the public os-headers (the "IncludeOS ABI"), which is what you'll be including in the service.
  2. When the service gets built it will turn into object files, which gets statically linked with the os-library and other libraries. Only the objects actually needed by the service will be linked, turning it all into one minimal elf-binary, `your_service`, with OS included.
  4. The utility `./vmbuild` (which was also compiled if needed) combines the installed bootloader and `your_service` into a disk image called `your_service.img`. At this point the bootloader gets the size and location of the service hardcoded into it.
  5. Now Qemu can start with that image as hard disk.

Inspect the [Makefile](./src/Makefile) and [linker script, linker.ld](./src/linker.ld) for more information about how the build happens, and [vmbuild/vmbuild.cpp](./vmbuild/vmbuild.cpp) for how the image gets constructed.

### The boot process goes like this:
  1. BIOS loads [bootloader.asm](./src/bootloader.asm), starting at `_start`. 
  2. The bootloader sets up segments, switches to protected mode, loads the service (an elf-binary `your_service` consisting of the OS classes, libraries and your service) from disk.
  3. The bootloader hands over control to the OS, which starts at the `_start` symbol inside [kernel_boot.cpp](src/kernel_boot.cpp). 
  4. The OS initializes `.bss`, calls clobal constructors (`_init`), and then calls `main` which just calls `OS::start` in [class_os.cpp](./src/class_os.cpp), which again sets up interrupts, initializes devices +++, etc. etc.
  5. Finally the OS class (still `OS::start`) calls `Service::start()`, inside your service, handing over control to you.


## Tools 

### Debugging with bochs
* If you want to debug the bootloader, or inspect memory, registers, flags etc. using a GUI, you need to install [bochs](http://bochs.sourceforge.net/), since `gdb` only works for objects with debugging symbols, which we don't have for our bootloader . See `./etc/bochs_installation.sh` for build options, and `./etc/.bochsrc` for an example config. file, (which specifies a <1MB disk).


### Using VirtualBox for development
  * VirtualBox does not support nested virtualization (a [ticket](https://www.virtualbox.org/ticket/4032) has been open for 5 years). This means you can't use the kvm module to run IncludeOS from inside vritualbox, but you can use Qemu directly, so developing for IncludeOS in a virtualbox vm works. It will be slower, but a small VM still boots in no time. For this reason, this install script does not require kvm or nested virtualization.
  * You might want to install Virtual box vbox additions, if want screen scaling. The above provides the requisites for this (compiler stuff). 


## Q&A

* Why can't we just start with implementing `int main(...)`?
      * We could, but the function signature wouldn't make any sense; we have only one process and no shell, so there's no place to return to, or to pass in arguments from.
* Why can't we have more than one process? 
       * IncludeOS is intended to be the "elastic" part of an elastic cloud service. That means we might want a whole lot of IncludeVM's going up and down, and adding any feature *x* to a vm *v* will give us *(vm+x)\*n* instad of just *vm\*n*. And we don't want to pay for anything more than we need, especially not inside the part that's supposed to scale.
