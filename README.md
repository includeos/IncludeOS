![IncludeOS Logo](./doc/logo.png)
================================================

**Update**: Looking for [Acorn](https://github.com/includeos/acorn/), the innovative web server appliance we [demoed at CppCon](https://www.youtube.com/watch?v=t4etEwG2_LY)? Built using [Mana](https://github.com/includeos/mana), the new C++ Web Application Framework for IncludeOS. Both [Acorn](https://github.com/includeos/acorn/) and [Mana](https://github.com/includeos/mana) are free and open source, check them out right here on GitHub!

IncludeOS is an includable, minimal [unikernel](https://en.wikipedia.org/wiki/Unikernel) operating system for C++ services running in the cloud. Starting a program with `#include <os>` will literally include a tiny operating system into your service during link-time.

The build system will:
* link your service with the necessary OS objects into a single binary
* attach a boot loader
* combine everything into a self-contained bootable disk image, ready to run on almost any modern hypervisor.

IncludeOS is free software, with "no warranties or restrictions of any kind".

[![Early Prototype](https://img.shields.io/badge/IncludeOS-v0.8.1-yellow.svg)](https://github.com/hioa-cs/IncludeOS/releases)
[![Apache v2.0](https://img.shields.io/badge/license-Apache%20v2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)
[![Join the chat at https://gitter.im/hioa-cs/IncludeOS](https://badges.gitter.im/hioa-cs/IncludeOS.svg)](https://gitter.im/hioa-cs/IncludeOS?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

**Note:** *IncludeOS is under active development. Anything may change at any time. The public API should not be considered stable.*

## Build status

|        | Build from bundle                                                                                                                                             |
|--------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Master | [![Build Status](https://jenkins.includeos.org/buildStatus/icon?job=shield_master_bundle)](https://jenkins.includeos.org/job/shield_master_bundle/) |
| Dev    | [![Build Status](https://jenkins.includeos.org/buildStatus/icon?job=shield_dev_bundle)](https://jenkins.includeos.org/job/shield_dev_bundle/)      |

### Key features

* **Extreme memory footprint**: A minimal bootable image, including bootloader, operating system components and a complete C++ standard library is currently 707K when optimized for size.
* **KVM and VirtualBox support** with full virtualization, using [x86 hardware virtualization](https://en.wikipedia.org/wiki/X86_virtualization), available on any modern x86 CPUs). In principle IncludeOS should run on any x86 hardware platform, even on a physical x86 computer, given appropriate drivers. Officially, we develop for- and test on [Linux KVM](http://www.linux-kvm.org/page/Main_Page), which power the [OpenStack IaaS cloud](https://www.openstack.org/), and [VirtualBox](https://www.virtualbox.org), which means that you can run your IncludeOS service on both Linux, Microsoft Windows and Mac OS X.
* **C++11/14 support**
    * Full C++11/14 language support with [clang](http://clang.llvm.org) v3.8 and later.
    * Standard C++ library (STL) [libc++](http://libcxx.llvm.org) from [LLVM](http://llvm.org/).
    * Exceptions and stack unwinding (currently using [libgcc](https://gcc.gnu.org/onlinedocs/gccint/Libgcc.html)).
    * *Note:* Certain language features, such as threads and filestreams are currently missing backend support.
* **Standard C library** using [newlib](https://sourceware.org/newlib/) from [Red Hat](http://www.redhat.com/).
* **Virtio Network driver** with DMA. [Virtio](https://www.oasis-open.org/committees/tc_home.php?wg_abbrev=virtio) provides a highly efficient and widely supported I/O virtualization. We are working towards the new [Virtio 1.0 OASIS standard](http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html)
* **A highly modular TCP/IP-stack**.

A longer list of features and limitations is on the [wiki feature list](https://github.com/hioa-cs/IncludeOS/wiki/Features)

## Getting started

### Install libraries

**NOTE:** The script will install packages and create a network bridge, and thus will ask for sudo access.

```
    $ git clone https://github.com/hioa-cs/IncludeOS
    $ cd IncludeOS
    $ ./install.sh
```

**The script will:**

* Install the required dependencies: `curl make clang-3.8 nasm bridge-utils qemu`.
* Download the latest binary release bundle from github.
* Unzip the bundle to `$INCLUDEOS_INSTALL_LOC` (defaults to `$HOME`).
* Create a network bridge called `include0`, for tap-networking.
* Build the vmbuilder, which turns your service into a bootable image.
* Copy `vmbuild` and `qemu-ifup` from the repo, over to `$INCLUDEOS_HOME`.

Detailed installation instructions for [Vagrant](https://github.com/hioa-cs/IncludeOS/wiki/Vagrant), [OS X](https://github.com/hioa-cs/IncludeOS/wiki/OS-X) and [Ubuntu](https://github.com/hioa-cs/IncludeOS/wiki/Ubuntu) are available in the Wiki, as well as instructions for [building everything from source](https://github.com/hioa-cs/IncludeOS/wiki/Ubuntu#b-completely-build-everything-from-source-slow).

### Testing the installation

A successful setup enables you to build and run a virtual machine. Running:

```
    $ ./test.sh
```

will build and run [this example service](./examples/demo_service/service.cpp).

More information is [available on the wiki](https://github.com/hioa-cs/IncludeOS/wiki/Testing-the-example-service).

### Writing your first service

1. Copy the [./seed](./seed) directory to a convenient location like `~/your_service`. Then, just start implementing the `Service::start` function in the `Service` class, located in [your_service/service.cpp](./seed/service.cpp) (Very simple example provided). This function will be called once the OS is up and running.  
2. Enter the name of your service in the first line of the [seed Makefile](./seed/Makefile). This will be the base for the name of the final disk image.

**Example:**

```
    $ cp -r seed ~/my_service
    $ cd ~/my_service
    $ emacs service.cpp
    ... add your code
    $ make
    $ ./run.sh my_service.img
```

Take a look at the [examples](./examples) and the [tests](./test). These all started out as copies of the same seed.

## Contributing to IncludeOS

IncludeOS is being developed on GitHub. Create your own fork, send us a pull request, and [chat with us on Gitter](https://gitter.im/hioa-cs/IncludeOS). Please read the [Guidelines for Contributing to IncludeOS](https://github.com/hioa-cs/IncludeOS/wiki/Contributing-to-IncludeOS).

**Important**: Send your pull requests to the `dev` branch. It is ok if your pull requests come from your master branch.

## C++ Guidelines

We want to adhere as much as possible to the [ISO C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines). When (not if) you find code in IncludeOS which doesn't adhere, please let us know in the [issue tracker](https://github.com/hioa-cs/IncludeOS/issues) - or even better, fix it in your own fork and send us a [pull-request](https://github.com/hioa-cs/IncludeOS/pulls).

## Read more on the wiki

We're trying to grow a Wiki, and some questions might already be answered here in the [FAQ](https://github.com/hioa-cs/IncludeOS/wiki/FAQ).

See the [Wiki front page](https://github.com/hioa-cs/IncludeOS/wiki) for a complete introduction, system overview, and more detailed guides.
