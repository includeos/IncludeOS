![IncludeOS](./doc/IncludeOS_logo.png)
================================================
[![Hex.pm](https://img.shields.io/hexpm/l/plug.svg)](LICENSE.md)
[![Build Status](http://jenkins.includeos.org/job/complete_bundle_master/badge/icon)](http://jenkins.includeos.org/job/complete_bundle_master/)


IncludeOS is an includeable, minimal library operating system for C++ services running in the cloud. Starting a program with `#include <os>`, will literally include a whole little operating system into your service during link-time. The build system will link your service and only the necessary OS objects into a single binary, attach a boot loader and combine all that into a self-contained bootable disk image, ready to run on a modern hypervisor. In other words, it's a [Unikernel](https://en.wikipedia.org/wiki/Unikernel) written from scratch, employing x86 hardware virtualization, with no dependencies except for the virtual hardware.

# It's a research prototype!
IncludeOS is not production ready, not feature complete, and very much a work in progress. However, it has been shown to outperform Linux virtual machines in terms of CPU usage by 5-20%, and memory usage by orders of magnitude, running a simple DNS service (both platforms ran the same binary). Preliminary performance results and a (now outdated) overview of IncludeOS will appear in an [IEEE CloudCom 2015](http://2015.cloudcom.org/) paper, titled *IncludeOS: A resource efficient unikernel for cloud services*. A [preprint is available here](doc/papers/IncludeOS_IEEE_CloudCom2015_PREPRINT.pdf), but for any [citations please refer to the publications seciton](https://github.com/hioa-cs/IncludeOS/wiki/Publications) in the [Wiki](https://github.com/hioa-cs/IncludeOS/wiki). 

### Key features
* **Extreme memory footprint**: A minimal bootable image, including bootloader, operating system components and a complete C++ standard library is currently 693K when optimized for size.
* **KVM and VirtualBox support** with full virtualization, using [x86 hardware virtualization](https://en.wikipedia.org/wiki/X86_virtualization) whenever available (it is on most modern x86 CPU's). In principle IncludeOS should run on any x86 hardware platform, even on a physical x86 computer, given appropriate drivers. Officially, we develop for- and test on [Linux KVM](http://www.linux-kvm.org/page/Main_Page), which power the [OpenStack IaaS cloud](https://www.openstack.org/), and [VirtualBox](https://www.virtualbox.org), which means that you can run your IncludeOS service on both Linux, Microsoft Windows and Apple OS X. 
* **C++11/14 support**
    * Full C++11/14 language support with [clang](http://clang.llvm.org) v3.6 and later.
    * Standard C++ library** (STL) [libc++](http://libcxx.llvm.org) from [LLVM](http://llvm.org/)
    * Exceptions and stack unwinding (currently using [libgcc](https://gcc.gnu.org/onlinedocs/gccint/Libgcc.html))
    * *Note:* Certain language features, such as threads and filestreams are currently missing backend support. 
* **Standard C library** using [newlib](https://sourceware.org/newlib/) from [Red Hat](http://www.redhat.com/)
* **Virtio Network driver** with DMA. [Virtio](https://www.oasis-open.org/committees/tc_home.php?wg_abbrev=virtio) provides a highly efficient and widely supported I/O virtualization. Like most implementations IncludeOS currently uses "lecacy mode", but we're working towards the new [Virtio 1.0 OASIS standard](http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html)
* **A highly modular TCP/IP-stack** written from scratch, still under heavy development.
    * TCP: Just enough to serve HTTP
    * UDP: Enough to support a high performance DNS service
    * ICMP: Enough to answer ping
    * ARP
    * Ethernet
    * IPv6 support under active development
* **Completely silent while idling**. As we documented in our [IEEE CloudCom 2013 paper](http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=6753801), running a regular interval timer for concurrency inside a virtual machine will impose a significant CPU-load on hypervisors running many virtual machines. IncludeOS disables the timer interrupts completely when idle, making it use no CPU at all. This makes IncludeOS services well suited for resource saving through overbooking schemes. 
* **Node.js-style callback-based programming** - everything happens in one efficient thread with no I/O blocking or unnecessary guest-side context switching.
* **No race conditions**. Delegated IRQ handling makes race conditions in "userspace" "impossible". ...unless you implement threads yourself (you have the access) or we do.
* **No virtual memory overhead** One service pr. VM means no need for virtual address spaces, and no overead due to address translation. Everything happens in a single, ring 0 address space. This has high positive impact on memory performance on some systems, but less so on newer CPU's with good hardware support for [nested paging](https://en.wikipedia.org/wiki/Second_Level_Address_Translation).
* **All the guns and all the knives:** 
  * IncludeOS services run in ring 0, in a single address space without protection. That's a lot of power to play with. For example: Try `asm("hlt")` in a normal userspace program - then try it in IncludeOS. Explain to the duck exactly what's going on ... and it will tell you why Intel made VT-x (Yes IBM was way behind Alan Turing). That's a virtualization gold nugget, in reward of your mischief. If you believe in these kinds of lessons, there's always more [Fun with Guns and Knives](https://github.com/hioa-cs/IncludeOS/wiki/Fun-with-Guns-and-Knives).
  * *Hold your forces! I and James Gosling strongly object to guns and knives!*
    * For good advice on how not to use these powers, look to the [Wisdom of the Jedi Council](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md). 
    * If you found the gold nugget above, you'll know that the physical CPU protects you from others - and others from you. And that's a pretty solid protection compared to, say, [openssl](https://xkcd.com/1354/). If you need protection from yourself, that too can be gained by aquiring the 10 000 lines of [Wisdom from the Jedi Council](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md), or also from our friends at [Mirage](http://mirage.io) ;-) 
    * *Are the extra guns and knives really features?* For explorers, yes. For a Joint Strike Fighter autopilot? Noooo. You need [even more wisdom](http://www.stroustrup.com/JSF-AV-rules.pdf) for that.

### Limitations 
If it's not listed under features, you can pretty much assume we don't have it yet. See the [Roadmap](https://github.com/hioa-cs/IncludeOS/wiki/Roadmap) for our current plan.

# Try it out!

## Prerequisites for building IncludeOS VM's
  * A machine with at least 1024 MB memory - a VirtualBox VM will work fine
     * (At least for the full compilation from source, we've found 512 MB to be insufficient). 
  * Ubuntu 14.04 LTS x86_64, Vanilla 
     * In order to support VGA graphics inside the VM, we recommend a lightweight GUI, such as  [lubuntu](http://lubuntu.net/blog/lubuntu-1404-trusty-tahr-released) which runs great inside a virtual machine.
         * *NOTE:* Graphics is by no means necessary, as all IncludeOS output by default will be routed to the serial port, and in Qemu, that ends up right on `stdout`.
     * The install scripts may very well work on other flavours on Linux, but we haven't tried. Please let us know if you do.
  * You'll need `git` to clone from github.

(I'm using an Ubuntu VM, running virtualbox inside a mac.)

Once you have a system with the prereqs (virtual or not), everything should be set up by:

## A) Install libraries from binary bundle (fast)
    $ sudo apt-get install git
    $ git clone https://github.com/hioa-cs/IncludeOS
    $ cd IncludeOS
    $ ./etc/install_from_bundle.sh

**The script will:**
* Install the required dependencies: `curl make clang-3.6 nasm bridge-utils qemu`
* Download the latest binary release bundle from github, using the github API.
* Unzip the bundle to `$INCLUDEOS_INSTALL_LOC` - which you can set in advance, or which defaults to `$HOME`
* Create a network bridge called `include0`, for tap-networking
* Build the vmbuilder, which turns your service into a bootable image
* Copy `vmbuild` and `qemu-ifup` from the repo, over to `$INCLUDEOS_HOME`

**Time:**
About a miniute or two (On a 4-core virtualbox Ubuntu VM, runing on a 2015 MacBook Air)

## B) Completely build everything from source (slow)
    $ sudo apt-get install git
    $ git clone https://github.com/hioa-cs/IncludeOS
    $ cd IncludeOS
    $ ./install.sh
    
**The script will:**
* Install all the tools required for building IncludeOS, and all libraries it depends on:
  * `build-essential make nasm texinfo clang-3.6 cmake ninja-build subversion zlib1g-dev libtinfo-dev`
* Build a GCC cross compiler along the lines of the [osdev howto](http://wiki.osdev.org/GCC_Cross-Compiler) which we really only need to build `libgcc` and `newlib`.
* Build [Redhat's newlib](https://sourceware.org/newlib/) using the cross compiler, and install it according to `./etc/build_newlib.sh`. The script will also install it to the mentioned location.
* Build a 32-bit version of [LLVM's libc++](http://libcxx.llvm.org/) tailored for IncludeOS. 
* Build and install the IncludeOS library, which your service will be linked with.
* Build and install the `vmbuild` tool, which turns your service into a bootable disk image.

**Time:** 
On a VM with 2 cores and 4 GB RAM, running Ubuntu 14.04, install.sh takes about 33 minutes, depending on bandwidth.

**NOTE:** Both scripts will install packages, and as such parts will require sudo access.

### Testing the installation

A successful setup should enable you to build and run a virtual machine. Running the test-script:

    IncludeOS$ ./test.sh 

will build and run a [this example service](./examples/demo_service/service.cpp). 

**Things to note**
* The defalut test script will only work on Linux, and uses Qemu (with KVM if available). To run IncludeOS directly on virtualbox, see `etc/vboxrun.sh`
* There is no shell! IncludeOS is a unikernel, meaning it will only run one process. Think of an IncludeOS VM as a local process.
* There is no VGA! So, nothong will show up on the "screen" if you're using a GUI (i.e. if you run IncludeOS directly in virtualbox) We did write a vga module at one point, but we never use it; output to serial port works very well. We'll add VGA support in the future, as a package.
* You should be able to ping the vm. Its IP-address will be stated in the boot-time output from IncldueOS 
* You should also be able to open a simple webpage on the vm, by entering the IP into a browser, inside the development machine.
* How to get out? The test script starts [qemu](http://wiki.qemu.org/Main_Page) with the `--nographics`-option. This will by default reroute stdin and stdout to the terminal. To exit the virtual machine, you can go via the [Qemu monitor](https://en.wikibooks.org/wiki/QEMU/Monitor#Virtual_machine). The command for entering the monitor is `Ctrl+a c`, or to exit directly, `ctrl+a x`. 
   * *NOTE*: This keyboard shortcut may not work if you're interacting with your development environment is via a VirtualBox GUI, over putty, inside a `screen` etc. If you find a good solution for a certain platform (i.e. putty to virtualbox on windows), please let us know so we can update the wiki.

## Write a service

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

## Tools 

### Debugging with bochs
* If you want to debug the bootloader, or inspect memory, registers, flags etc. using a GUI, you need to install [bochs](http://bochs.sourceforge.net/), since `gdb` only works for objects with debugging symbols, which we don't have for our bootloader . See `./etc/bochs_installation.sh` for build options, and `./etc/.bochsrc` for an example config. file, (which specifies a <1MB disk).


### Using VirtualBox for development
  * VirtualBox does not support nested virtualization (a [ticket](https://www.virtualbox.org/ticket/4032) has been open for 5 years). This means you can't use the kvm module to run IncludeOS from inside vritualbox, but you can use Qemu directly, so developing for IncludeOS in a virtualbox vm works. It will be slower, but a small VM still boots in no time. For this reason, this install script does not require kvm or nested virtualization.
  * You might want to install Virtual box vbox additions, if want screen scaling. The above provides the requisites for this (compiler stuff). 

### C++ Guidelines
We are currently far from it, but in time we'd like to adhere more or less to the [ISO C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines), maintained by the [Jedi Council](https://isocpp.org/). When (not if) you find code in IncludeOS, which doesn't adhere, please let us padawans know, in the issute-tracker - or even better, fix it in your own fork, and send us a pull-request. 

## Read more on the wiki
We're trying to grow a Wiki, and some questions might allready be answered here in the [FAQ](https://github.com/hioa-cs/IncludeOS/wiki/FAQ). 

See the [Wiki front page](https://github.com/hioa-cs/IncludeOS/wiki) for a complete introduction, system overview, and more detailed guies.



