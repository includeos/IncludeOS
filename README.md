![IncludeOS Logo](./doc/logo.png)
================================================

**Update**: Check out [Acorn](examples/acorn/), the innovative web server appliance we [demoed at CppCon](https://www.youtube.com/watch?v=t4etEwG2_LY). Built using [Mana](lib/mana/), the new C++ Web Application Framework for IncludeOS.

A *live demo* of Acorn can be found at [acorn2.unofficial.includeos.io](http://acorn2.unofficial.includeos.io) (sporadically unavailable)

**IncludeOS** is an includable, minimal [unikernel](https://en.wikipedia.org/wiki/Unikernel) operating system for C++ services running in the cloud. Starting a program with `#include <os>` will literally include a tiny operating system into your service during link-time.

The build system will:
* link your service with the necessary OS objects into a single binary
* attach a boot loader
* combine everything into a self-contained bootable disk image, ready to run on almost any modern hypervisor.

IncludeOS is free software, with "no warranties or restrictions of any kind".

[![Early Prototype](https://img.shields.io/badge/IncludeOS-v0.10.0-yellow.svg)](https://github.com/hioa-cs/IncludeOS/releases)
[![Apache v2.0](https://img.shields.io/badge/license-Apache%20v2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)
[![Join the chat at https://gitter.im/hioa-cs/IncludeOS](https://badges.gitter.im/hioa-cs/IncludeOS.svg)](https://gitter.im/hioa-cs/IncludeOS?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

**Note:** *IncludeOS is under active development. The public API should not be considered stable.*

## Build status

|        | Build from bundle | Integration tests |
|--------|-------------------|-------------------|
| Master | [![Build Status](https://jenkins.includeos.org/buildStatus/icon?job=shield_master_bundle)](https://jenkins.includeos.org/job/shield_master_bundle/) |  Coming soon |
| Dev    | [![Build Status](https://jenkins.includeos.org/buildStatus/icon?job=shield_dev_bundle)](https://jenkins.includeos.org/job/shield_dev_bundle/)      | [![Jenkins tests](https://img.shields.io/jenkins/t/https/jenkins.includeos.org/shield_dev_integration_tests.svg)](https://jenkins.includeos.org/job/shield_dev_integration_tests/) |

### Key features

* **Extreme memory footprint**: A minimal bootable image, including bootloader, operating system components and a complete C++ standard library is currently 707K when optimized for size.
* **KVM and VirtualBox support** with full virtualization, using [x86 hardware virtualization](https://en.wikipedia.org/wiki/X86_virtualization), available on any modern x86 CPUs). In principle IncludeOS should run on any x86 hardware platform, even on a physical x86 computer, given appropriate drivers. Officially, we develop for- and test on [Linux KVM](http://www.linux-kvm.org/page/Main_Page), which power the [OpenStack IaaS cloud](https://www.openstack.org/), and [VirtualBox](https://www.virtualbox.org), which means that you can run your IncludeOS service on both Linux, Microsoft Windows and macOS.
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

### Set custom location and compiler

By default the project is installed to /usr/local/includeos.

However, it is recommended to choose a custom location as well as select the compiler we want clang to find.

To do this we can edit ~/.bashrc (in the home folder), adding these lines at the end of the file:

```
    export CC=/usr/bin/clang-3.8
    export CXX=/usr/bin/clang++-3.8
    export INCLUDEOS_PREFIX=<HOME FOLDER>/includeos
    export PATH=$PATH:$INCLUDEOS_PREFIX/bin
```

This will also crucially make the boot program visible globally, so that you can simply run ```boot <myservice>``` inside any service folder.

### Install libraries

**NOTE:** The script will install packages and create a network bridge.

```
    $ git clone https://github.com/hioa-cs/IncludeOS
    $ cd IncludeOS
    $ ./install.sh
```

**The script will:**

* Install the required dependencies: `curl make clang-3.8 nasm bridge-utils qemu`.
* Create a network bridge called `bridge43`, for tap-networking.
* Build IncludeOS with CMake:
  * Download the latest binary release bundle from github together with the required git submodules.
  * Unzip the bundle to the current build directory.
  * Build several tools used with IncludeOS, including vmbuilder, which turns your service into a bootable image.
  * Install everything in `$INCLUDEOS_PREFIX/includeos` (defaults to `/usr/local`).

Configuration of your IncludeOS installation can be done inside `build/` with `ccmake ..`.

### Testing the installation

A successful setup enables you to build and run a virtual machine. Running:

```
    $ ./test.sh
```

will build and run [this example service](./examples/demo_service/service.cpp).

More information is [available on the wiki](https://github.com/hioa-cs/IncludeOS/wiki/Testing-the-example-service).

### Writing your first service

1. Copy the [./seed/service](./seed/service) directory to a convenient location like `~/your_service`. Then, just start implementing the `Service::start` function in the `Service` class, located in [your_service/service.cpp](./seed/service/service.cpp) (Very simple example provided). This function will be called once the OS is up and running.
2. Update the [CMakeLists.txt](./seed/service/CMakeLists.txt) to specify the name of your project, enable any needed drivers or plugins, etc.

**Example:**

```
    $ cp -r seed/service ~/my_service
    $ cd ~/my_service
    $ emacs service.cpp
    ... add your code
    $ mkdir build && cd build
    $ cmake ..
    $ make
    $ boot my_service
```

Take a look at the [examples](./examples) and the [tests](./test). These all started out as copies of the same seed.

## Contributing to IncludeOS

IncludeOS is being developed on GitHub. Create your own fork, send us a pull request, and [chat with us on Gitter](https://gitter.im/hioa-cs/IncludeOS). Please read the [Guidelines for Contributing to IncludeOS](https://github.com/hioa-cs/IncludeOS/wiki/Contributing-to-IncludeOS).

**Important: Send your pull requests to the `dev` branch**. It is ok if your pull requests come from your master branch.

## C++ Guidelines

We want to adhere as much as possible to the [ISO C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines). When you find code in IncludeOS which doesn't adhere, please let us know in the [issue tracker](https://github.com/hioa-cs/IncludeOS/issues) - or even better, fix it in your own fork and send us a [pull-request](https://github.com/hioa-cs/IncludeOS/pulls).

## Read more on the wiki

We're trying to grow a Wiki, and some questions might already be answered here in the [FAQ](https://github.com/hioa-cs/IncludeOS/wiki/FAQ).

See the [Wiki front page](https://github.com/hioa-cs/IncludeOS/wiki) for a complete introduction, system overview, and more detailed guides.
