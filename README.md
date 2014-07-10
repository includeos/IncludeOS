IncludeOS-DevEnv
================

Scripts or whatever needed to set up an IncludeOS development environment


# Prerequisites 

  * Ubuntu 14.04 LTS, Vanilla (I use lubuntu, to support VM graphics if necessary)
  * Git
  
# Installation

Once you have a system with the prereqs (virtual or not), everything should be set up by:

    $sudo apt-get install git
    $git clone https://github.com/hioa-cs/IncludeOS-DevEnv
    $cd IncludeOS-DevEnv
    $sudo ./install.sh

# Testing the installation

A successful setup should enable you to build and run a virtual machine. Some code for a very simple one is provided. The command

    $./test.sh 

will build and run a VM for you, and let you know if everything worked out. 

# VirtualBox config
  * VirtualBox does not support nested virtualization (a [ticket](https://www.virtualbox.org/ticket/4032) has been open for 5 years). This means you can't use the kvm module, but you can use Qemu directly. It will be slower, but a small VM still boots in no time. For this reason, this install script does not require kvm or nested virtualization.
  * You might want to install Virtual box vbox additions, if want screen scaling. The above provides the requisites for this (compiler stuff). 
