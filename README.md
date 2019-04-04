![IncludeOS Logo](./etc/logo.png)
================================================

**IncludeOS** is an includable, minimal [unikernel](https://en.wikipedia.org/wiki/Unikernel) operating system for C++ services running in the cloud and on real HW. Starting a program with `#include <os>` will literally include a tiny operating system into your service during link-time.

IncludeOS is free software, with "no warranties or restrictions of any kind".

[![Pre-release](https://img.shields.io/github/release-pre/includeos/IncludeOS.svg)](https://github.com/hioa-cs/IncludeOS/releases)
[![Apache v2.0](https://img.shields.io/badge/license-Apache%20v2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)
[![Join the chat](https://img.shields.io/badge/chat-on%20Slack-brightgreen.svg)](https://goo.gl/NXBVsc)

**Note:** *IncludeOS is under active development. The public API should not be considered stable.*


## <a name="features"></a> Key features

* **Extreme memory footprint**: A minimal bootable 64-bit web server, including operating system components and a anything needed from the C/C++ standard libraries is currently 2.5 MB.
* **KVM, VirtualBox and VMWare support** with full virtualization, using [x86 hardware virtualization](https://en.wikipedia.org/wiki/X86_virtualization), available on most modern x86 CPUs. IncludeOS will run on any x86 hardware platform, even on a physical x86 computer, given appropriate drivers. Officially, we develop for- and test on [Linux KVM](http://www.linux-kvm.org/page/Main_Page), and VMWare [ESXi](https://www.vmware.com/products/esxi-and-esx.html)/[Fusion](https://www.vmware.com/products/fusion.html) which means that you can run your IncludeOS service on Linux, Microsoft Windows and macOS, as well as on cloud providers such as [Google Compute Engine](http://www.includeos.org/blog/2017/includeos-on-google-compute-engine.html), [OpenStack](https://www.openstack.org/) and VMWare [vcloud](https://www.vmware.com/products/vcloud-suite.html).
* **Instant boot:** IncludeOS on Qemu/kvm boots in about 300ms but IBM Research has also integrated IncludeOS with [Solo5/uKVM](https://github.com/Solo5/solo5), providing boot times as low as 10 milliseconds.
* **Modern C++ support**
    * Full C++11/14/17 language support with [clang](http://clang.llvm.org) 5 and later.
    * Standard C++ library (STL) [libc++](http://libcxx.llvm.org) from [LLVM](http://llvm.org/).
    * Exceptions and stack unwinding (currently using [libgcc](https://gcc.gnu.org/onlinedocs/gccint/Libgcc.html)).
    * *Note:* Certain language features, such as threads and filestreams are currently missing backend support but is beeing worked on.
* **Standard C library** using [musl libc](http://www.musl-libc.org/).
* **Virtio and vmxnet3 Network drivers** with DMA. [Virtio](https://www.oasis-open.org/committees/tc_home.php?wg_abbrev=virtio) provides a highly efficient and widely supported I/O virtualization. vmxnet3 is the VMWare equivalent.
* **A highly modular TCP/IP-stack**.

A longer list of features and limitations can be found on our [documentation site](http://includeos.readthedocs.io/en/latest/Features.html).

## Contents

- [Getting started](#getting_started)
  - [Dependencies](#dependencies)
  - [Hello world](#hello_world)
  - [Getting started developing packages](#develop_pkg)
- [Managing dependencies](#repositories_and_channels)
- [Building with IncludeOS in editable mode](#editing_includeos)
- [Contributing to IncludeOS](#contribute)
- [C++ Guidelines](#guideline)
- [Security contact](#security)

## <a name="getting_started"></a> Getting started

### <a name="dependencies"></a> Dependencies

For building IncludeOS services you will need:

* [The conan package manager](https://docs.conan.io/en/latest/installation.html) (1.13.1 or newer)
* cmake, make, nasm (x86/x86_64 only)
* clang, or alternatively gcc on linux. Prebuilt packages are available for clang 6.0 and gcc 7.3.

To boot VMs locally with our tooling you will also need:

* qemu
* python3 packages: psutil, jsonschema

The following command will configure conan to use our build profiles and remote repositories. (**Note:** this overwrites any existing conan configuration. Set `CONAN_USER_HOME` to create a separate conan home folder for testing.)

```text
$ conan config install https://github.com/includeos/conan_config.git
```
If you prefer to set up conan manually the full configuration can be found in the [conan_config](https://github.com/includeos/conan_config.git)-repository.

#### Ubuntu

```text
$ apt-get install python3-pip python3-dev git cmake clang-6.0 gcc nasm make qemu
$ pip3 install setuptools wheel conan psutil jsonschema
$ conan config install https://github.com/includeos/conan_config.git
```

#### macOS
If you have [homebrew](https://brew.sh/) you can use our `brew tap` to install the dependencies.

```text
$ brew tap includeos/includeos
$ brew install includeos
$ conan config install https://github.com/includeos/conan_config.git
```

### <a href="hello_world"></a> Hello World

First select an appropriate [conan profile](https://docs.conan.io/en/latest/reference/profiles.html) for the target you want to boot on. `conan profile list` will show the profiles available, including the ones installed in the previous step. When developing for the machine you're currently on, Linux users can typically use `clang-6.0-linux-x86_64`, and MacOS users can use `clang-6.0-macos-x86_64`. You can also make your own.

The following steps let you build and boot the IncludeOS hello world example.

```text
$ git clone https://github.com/includeos/hello_world.git
$ mkdir your_build_dir && cd "$_"
$ conan install ../hello_world -pr <your_conan_profile>
$ source activate.sh
$ cmake ../hello_world
$ cmake --build .
$ boot hello
```
You can use the [hello world repo](https://github.com/includeos/hello_world) as a starting point for developing your own IncludeOS services. For more advanced examples see the [examples repo](https://github.com/includeos/demo-examples) or the integration tests (under ./IncludeOS/test/\*/integration).

Once you're done `$ source deactivate.sh` will reset the environment to its previous state.

### <a name="develop_pkg"></a> Getting started developing packages

If you are interested in editing/building our dependency packages on your own, you can checkout our repositories at [includeos/](https://github.com/includeos/). Some of our tools and libraries are listed below under [Tools and Libraries](#libs_tools). You can find the external dependency recipes at [includeos/conan](https://github.com/includeos/conan). Currently we build with `clang-6` and `gcc-7.3.0` compiler toolchains. It is expected that these are already installed on your system (see [Dependencies](#dependencies) for details).

## <a href="repositories_and_channels"></a> Managing dependencies

Prebuilt packages are uploaded to our [bintray repository](https://bintray.com/includeos).

We upload to two channels:

- `stable`: this channel has all the stable packages.
- `latest`: this channel will have the latest packages in development/test phase (including stable releases).

> **Note:** We only guarantee that the **latest 10 packages** are kept in the `latest` channel. All `stable` packages will be kept in the stable channel unless proven unsafe. One suggested workaround is to copy packages into your own repository.

To set up our remote, we recommend following the steps listed in [Dependencies](#dependencies).

### Search

If you want to check if a package exists you can search for it with `conan search`. To list all the available packages on our remote `includeos`, you can use:

```text
$ conan search -r includeos
```

This should list all the packages we have uploaded to the includeos repository.

To find all the stable versions uploaded for a particular package:

```text
$ conan search -r includeos '<package_name>/*@includeos/stable'
```

### Prebuilt profiles
The profiles we are using for development can be found in the [includeos/conan_config](https://github.com/includeos/conan_config) repository under `conan_config/profiles/`.

The target profiles we have verified are the following:

- [clang-6.0-linux-x86](https://github.com/includeos/conan_config/tree/master/profiles/clang-6.0-linux-x86)
- [clang-6.0-linux-x86_64](https://github.com/includeos/conan_config/tree/master/profiles/clang-6.0-linux-x86_64)
- [gcc-7.3.0-linux-x86_64](https://github.com/includeos/conan_config/tree/master/profiles/gcc-7.3.0-linux-x86_64)
- [clang-6.0-macos-x86_64](https://github.com/includeos/conan_config/tree/master/profiles/clang-6.0-macos-x86_64)

These profiles should have prebuilt packages available and are tested in CI. If you create a custom profile (or use a different profile provided by us) the dependencies may have to be rebuilt locally.

## <a name="editing_includeos"></a> Building with IncludeOS in editable mode

If you are a kernel developer or are simple interested in fiddling with our kernel code, you can use the includeos-package in editable mode. When you rebuild the package will then automatically be updated so it can be used by other packages locally. This is useful when working with several interconnected components and you would like to test your changes across several libraries.

You can read more about how editable mode works in the [Conan documentation](https://docs.conan.io/en/latest/developing_packages/editable_packages.html).

Below we have written down a few steps to get you started with editable packages and IncludeOS.

> **Note:** Currently this is an experimental feature on conan version 1.13 and they have mentioned that for future releases the feature is subject to breaking changes.

Start by cloning the IncludeOS source code and create a `build` folder. You have to edit `etc/layout.txt` in the source code to point to the `build` folder you created, by updating the `build_dir` variable.

The layout will look similar to the example below. You only have to update `build_dir`.

```text
  {% set simple=true%}

  {% set build_dir='build' %}

  {% if simple==false %}
  {% set arch=settings.arch|string %}
  {% set platform=options.platform|string %}
  {% set build_dir=build_dir+'/'+arch+'/'+platform %}
  {% endif %}

  [bindirs]
  {#This is so that we can find os.cmake after including conanbuildinfo.cmake #}
  cmake

  [includedirs]
  {#This is to ensure the include directory in conanbuildinfo.cmake includes our API#}
  api

  [libdirs]
  {#This is so that we can find our libraries #}
  {{ build_dir }}/plugins
  {{ build_dir }}/drivers
  {{ build_dir }}/lib
  {{ build_dir }}/platform

  [resdirs]
  {#This is so that we can find ldscript and search for drivers plugins etc#}
  {{ build_dir }}

```
> **Note:** in the non simple form it is possible to have multiple build folders from the same source which allows multiple architectures and configurations to be tested from the same source however the complexity increases

You should now be able to set the package in editable mode. The following command will add the package as editable based on the specified layout. We inspect the package to get the version, as this has to match exactly.

```text
$ conan editable add . includeos/$(conan inspect -a version . | cut -d " " -f 2)@includeos/latest --layout=etc/layout.txt
```

The package is now in **editable mode** and any dependencies of IncludeOS will pick this IncludeOS package from your local cache.

Here is an example on how it looks when its pulled into cache as editable:

```text
$ conan editable list
  includeos/0.14.1-1052@includeos/latest
    Path: ~/git/IncludeOS
    Layout: ~/git/IncludeOS/etc/layout.txt
```

We are now ready to build the package. Assuming the build-folder is called `build` under the includeos source directory the following is enough.

```text
$ cd [includeos source root]
$ conan install -if build . -pr <conan_profile> (-o options like platform=nano etc)
$ conan build -bf build .
```

After making changes to the code you can rebuild the package with

```text
$ cd build && make
   or
$ cmake build --build
```

Once you have made your changes and the code is **finalized** you should verify that the conan package still builds. Remove the editable and do a conan create on the package:

```text
$ conan editable remove includeos/<version>@includeos/test
$ conan create <source_path> includeos/latest -pr <conan_profile>
```

## Libraries and tools

We have moved the libraries and tools created by IncludeOS outside the includeos-repository. You can now find them all in their own repositories inside the IncludeOS organization.

To build the libraries and tools, see build instructions in each repository. Typically, the instructions will be in the form:

```text
$ git clone https://github.com/includeos/mana.git
$ cd mana
$ conan create . includeos/latest -pr clang-6.0-linux-x86_64
```

<a name="libs_tools"></a> Below is a list of some of our Libraries/Tools:

- [Vmbuild](https://github.com/includeos/vmbuild) -
Vmbuild is an utility for building the IncludeOS virtual machines.

- [Vmrunner](https://github.com/includeos/vmrunner) -
Vmrunner is a utility developed for booting IncludeOS binanries.

- [Mana](https://github.com/includeos/mana) -
Mana is a web application framework which is used to build a IncludeOS webserver.
We have an example named [acorn](https://github.com/includeos/demo-examples/tree/master/acorn) which demonstrates mana's potential.

- [Microlb](https://github.com/includeos/microlb) -
Microlb is a library written for building the IncludeOS load balancer.
We have an example named [microlb](https://github.com/includeos/demo-examples/tree/master/microLB) which demonstrates our load balancer.

- [Diskbuilder](https://github.com/includeos/diskbuilder) -
Diskbuilder is a tool used for building disks for IncludeOS.

- [NaCl](https://github.com/includeos/NaCl) -
NaCl is the configuration language tool we have tailored for IncludeOS to allow users to configure various network settings such as firewall rules, vlans, ip configurations etc.

## <a name="contribute"></a> Contributing to IncludeOS

IncludeOS is being developed on GitHub. Create your own fork, send us a pull request, and [chat with us on Slack](https://goo.gl/NXBVsc). Please read the [Guidelines for Contributing to IncludeOS](http://includeos.readthedocs.io/en/latest/Contributing-to-IncludeOS.html).

## <a name="guideline"></a> C++ Guidelines

We want to adhere as much as possible to the [ISO C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines). When you find code in IncludeOS which doesn't adhere, please let us know in the [issue tracker](https://github.com/includeos/IncludeOS/issues) - or even better, fix it in your own fork and send us a [pull-request](https://github.com/includeos/IncludeOS/pulls).

[brew]: https://brew.sh/
[qemu]: https://www.qemu.org/

## <a name="security"></a> Security contact
If you discover a security issue in IncludeOS please avoid the public issue tracker. Instead send an email to security@includeos.org. For more information and encryption please refer to the [documentation](http://includeos.readthedocs.io/en/latest/Security.html).
