IncludeOS-DevEnv
================

Scripts or whatever needed to set up an IncludeOS development environment


# Prerequisites 
  * A machine with at least 1024 MB memory. (At least for ubuntu I ran out during compilation of toolchain with 512 MB)
  * Ubuntu 14.04 LTS, Vanilla (I use lubuntu, to support VM graphics if necessary)
  * Git

(I'm using an Ubuntu VM, running virtualbox inside a mac.)

# Installation

Once you have a system with the prereqs (virtual or not), everything should be set up by:

    $sudo apt-get install git
    $git clone https://github.com/hioa-cs/IncludeOS-DevEnv
    $cd IncludeOS-DevEnv
    $sudo ./install.sh

### But not really:
You also have to
* Build a cross compiler according to [osdev howto](http://wiki.osdev.org/GCC_Cross-Compiler) - I do it exactly like that, except that I use `/usr/local/cross/` as path, instead of `/opt/cross`. 
* Build [Redhat's newlib](https://sourceware.org/newlib/), using the cross compiler, and install it according to `./etc/build_newlib.sh`. The script will also install it, to the `exported` location.
* If you want to debug the bootloader, or inspect memory, registers, flags etc. using a GUI, you need to install [bochs](http://bochs.sourceforge.net/). See `./etc/bochs_installation.sh` for build options, and `./etc/.bochsrc` for an example config. file, (which specifies a <1MB disk).


# Testing the installation

A successful setup should enable you to build and run a virtual machine. Some code for a very simple one is provided. The command

    $./test.sh 

will build and run a VM for you, and let you know if everything worked out. 

# VirtualBox config
  * VirtualBox does not support nested virtualization (a [ticket](https://www.virtualbox.org/ticket/4032) has been open for 5 years). This means you can't use the kvm module, but you can use Qemu directly. It will be slower, but a small VM still boots in no time. For this reason, this install script does not require kvm or nested virtualization.
  * You might want to install Virtual box vbox additions, if want screen scaling. The above provides the requisites for this (compiler stuff). 


# Now what?
Once you've run a successful test (i.e. you got a lot of `....[PASS]` lines from Qemu followed by `>>> System halting - OK. Done.` ) you know IncludeOS works on your machine, and you can go ahead and tinker. Right now all the IncludeOS code is inside the `./vmbuilder` directory, and the kernel starts at the `_start` symbol inside `kernel_boot.cpp`. Inspect the [Makefile](./vmbuilder/Makefile) and [linker script](./vmbuilder/linker.ld) to get an idea of how the whole thing is built. d

## Helper scripts
There's a convenience script, `./vmbuilder/run.sh`, which has the Make-vmbuilder-qemu sequence laid out, with special options for debugging (It will add debugging symbols to the elf-binary and start qemu in debugging mode, ready for connection with `gdb`. More on this inside the script.)




