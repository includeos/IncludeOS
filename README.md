![IncludeOS Logo](./etc/logo.png)
================================================

**IncludeOS** is an includable, minimal [unikernel](https://en.wikipedia.org/wiki/Unikernel) operating system for C++ services running in the cloud. Starting a program with `#include <os>` will literally include a tiny operating system into your service during link-time.

The build system will:
* link your service with the necessary OS objects into a single binary
* attach a boot loader
* combine everything into a self-contained bootable disk image, ready to run on almost any modern hypervisor.

IncludeOS is free software, with "no warranties or restrictions of any kind".

[![Pre-release](https://img.shields.io/github/release-pre/hioa-cs/IncludeOS.svg)](https://github.com/hioa-cs/IncludeOS/releases)
[![Apache v2.0](https://img.shields.io/badge/license-Apache%20v2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)
[![Join the chat](https://img.shields.io/badge/chat-on%20Slack-brightgreen.svg)](https://goo.gl/NXBVsc)

**Note:** *IncludeOS is under active development. The public API should not be considered stable.*

##### Contents:

- [Key Features](#features)
- [Getting started with IncludeOS development](#getting_started)

- ###### Service Developers Howto:
 - [Building IncludeOS with dependencies from conan](#building_includeos)
 - [Building, starting and creating your first IncludeOS Service](#building_service)

- ###### Kernel Developers Howto:
 - [Getting IncludeOS in Editable mode](#editing_includeos)
 - [Getting started developing packages](#develop_pkg)

- ###### Other Info:
 - [Contributing to IncludeOS](#contribute)
 - [Our Website](https://www.includeos.org/)
 - [The Company](https://www.includeos.com/)
 - [C++ Guidelines](#guideline)
 - [Security contact](#security)


### <a name="features"></a> Key features

* **Extreme memory footprint**: A minimal bootable 64-bit web server, including operating system components and a anything needed from the C/C++ standard libraries is currently 2.5 MB.
* **KVM, VirtualBox and VMWare support** with full virtualization, using [x86 hardware virtualization](https://en.wikipedia.org/wiki/X86_virtualization), available on most modern x86 CPUs. IncludeOS will run on any x86 hardware platform, even on a physical x86 computer, given appropriate drivers. Officially, we develop for- and test on [Linux KVM](http://www.linux-kvm.org/page/Main_Page), and VMWare [ESXi](https://www.vmware.com/products/esxi-and-esx.html)/[Fusion](https://www.vmware.com/products/fusion.html) which means that you can run your IncludeOS service on Linux, Microsoft Windows and macOS, as well as on cloud providers such as [Google Compute Engine](http://www.includeos.org/blog/2017/includeos-on-google-compute-engine.html), [OpenStack](https://www.openstack.org/) and VMWare [vcloud](https://www.vmware.com/products/vcloud-suite.html).
* **Instant boot:** IncludeOS on Qemu/kvm boots in about 300ms but IBM Research has also integrated IncludeOS with [Solo5/uKVM](https://github.com/Solo5/solo5), providing boot times as low as 10 milliseconds.
* **Modern C++ support**
    * Full C++11/14/17 language support with [clang](http://clang.llvm.org) 5 and later.
    * Standard C++ library (STL) [libc++](http://libcxx.llvm.org) from [LLVM](http://llvm.org/).
    * Exceptions and stack unwinding (currently using [libgcc](https://gcc.gnu.org/onlinedocs/gccint/Libgcc.html)).
    * *Note:* Certain language features, such as threads and filestreams are currently missing backend support.
* **Standard C library** using [musl libc](http://www.musl-libc.org/).
* **Virtio and vmxnet3 Network drivers** with DMA. [Virtio](https://www.oasis-open.org/committees/tc_home.php?wg_abbrev=virtio) provides a highly efficient and widely supported I/O virtualization. vmxnet3 is the VMWare equivalent.
* **A highly modular TCP/IP-stack**.

A longer list of features and limitations can be found on our [documentation site](http://includeos.readthedocs.io/en/latest/Features.html).

___

### <a name="getting_started"></a> Getting started with IncludeOS development

The [IncludeOS](https://www.includeos.org/) conan recipes are developed with [Conan version 1.13.1](https://github.com/conan-io/conan/releases/tag/1.13.1) or newer.

For Mac OS ensure that you have a working installation of [brew](https://brew.sh/) to be able to install all dependencies.
</p>

##### Cloning the IncludeOS repository:
```
    $ git clone https://github.com/hioa-cs/IncludeOS
    $ cd IncludeOS
```

##### Dependencies

- Cmake
- Clang version: `6.0`
- GCC version: `gcc-7`
- [Conan](https://github.com/conan-io/conan)

### <a name="building_includeos"></a> Building IncludeOS with dependencies from conan

Conan uses [profiles](https://docs.conan.io/en/latest/reference/profiles.html)
to build packages. By default IncludeOS will build with `clang 6.0` if
`CONAN_PROFILE` is not defined.

##### Getting IncludeOS Conan Configs

We have set up a repository ([includeos/conan_config](https://github.com/includeos/conan_config)) that helps IncludeOS users configure all the necessary
conan settings. To configure using this repo just do:

```
  conan config install https://github.com/includeos/conan_config.git
```

This adds our remote in your conan remotes and also installs all the profiles we have in the repository for you.

###### Profiles
Profiles we are using for development can be found in [includeos/conan_config ](https://github.com/includeos/conan_config) repository under `conan_config/profiles/`.
If you have not used the `conan config install` command above, then to install the profiles, copy them over to your user `./conan/profiles` folder.

The target profiles we have verified are the following:
- [clang-6.0-linux-x86](https://github.com/includeos/conan_config/tree/master/profiles/clang-6.0-linux-x86)
- [clang-6.0-linux-x86_64](https://github.com/includeos/conan_config/tree/master/profiles/clang-6.0-linux-x86_64)
- [gcc-7.3.0-linux-x86_64](https://github.com/includeos/conan_config/tree/master/profiles/gcc-7.3.0-linux-x86_64)
- [clang-6.0-macos-x86_64](https://github.com/includeos/conan_config/tree/master/profiles/clang-6.0-macos-x86_64)

To ensure the profile/s has been installed do:

```
    $ conan profile list
```

Verify the content of your profile by:
```
    $ conan profile show <yourprofilename>
```

If your profile is on the list and contents are verified, you are set to use the
profile for building.

###### IncludeOS Bintray Repositories

The [Bintray](https://bintray.com/includeos) repository is where all the packages used to build IncludeOS
are uploaded. Adding the repo to your conan remotes will give you access to all
our packages developed for IncludeOS.

You check your repository remotes, do:

```
conan remote list
```
If the includeOS-Develop remote is not added do, you have to add it.

To add the IncludeOS-Develop conan Bintray repository to your conan remotes:

```
conan remote add includeos https://api.bintray.com/conan/includeos/includeos
```

##### Build IncludeOS

Finally to build IncludeOS do:

```
    $ conan create <path-to-conan-recipe> <conan-user>/<conan_channel> -pr <profile-name>
```

To build **IncludeOS on Linux** use profile named `clang-6.0-linux-x86_64` :

```
    $ conan create IncludeOS includeos/test -pr clang-6.0-linux-x86_64
```

To build **IncludeOS on Linux** for **Macos** use profile named `clang-6.0-macos-x86_64`.

_To build **IncludeOS on Mac** requires a number of other configurations, which
are in progress and will be updated soon._
<!-- TODO: test the whole process of macos and write down the process. -->


###### Searching Packages

if you want to check if a package exists you can search for it:

```
    conan search help
```

___

### <a name="building_service"></a>Building, starting and creating your first IncludeOS Service with Conan

Examples are now built with conan packages. The IncludeOS demo examples have now
been moved to [includeos/demo-examples](https://github.com/includeos/demo-examples.git).

To start build your first service clone the repository:

```
  $ git clone https://github.com/includeos/demo-examples.git
  $ cd demo-examples && ls
```

You will see a list of all the example services you can try building.

##### Building the demo service

To build the demo service, create a `build` folder inside the `demo_service` folder
and install with the profile you would like to use.

```
  $ cd demo_service
  $ mkdir build
  $ cd build
  $ conan install .. -pr <name-of-profile>
```

Installing with the chosen profile, fetches the profile configurations and Installs
the requirements in the `conanfile.txt`. If the required packages are not in the
local conan cache they are downloaded and installed. If all the packages required
are already in the local conan cache then it moves on to apply build requirements
and generates the required virtualenv scripts and cmake information.

Next to build the service do:

```
  $ cmake ..
  $ cmake --build .

```
Doing `cmake` configures and generates the build files and they are written to
the build folder. Then doing `cmake --build .` builds the target service. You
should see the last line as:

```
  [100%] Built target demo
```
`demo` is the name of this service for the demo_service. That's the name you will
use when starting the service. Do `ls` to see all the files created. There is also
an executable file named `demo` which is now the service you will start.


##### Starting a service

Now that you have all your build requirements ready, you can start the service.

However to start a service a good practice is to always activate the service
requirements;

```
  $ source activate.sh
```

Now you can boot the demo service with `boot <service-name>`.

```
  $ boot demo
```

This should start your demo service and show something along the following lines:

```
  ================================================================================
  IncludeOS  (x86_64 / 64-bit)
  +--> Running [ IncludeOS Demo Service ]
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Service started
  Made device_ptr, adding to machine
    [ Network ] Creating stack for VirtioNet on eth0 (MTU=1500)
       [ Inet ] Bringing up eth0 on CPU 0
  *** Basic demo service started ***
```

To build more examples follow the [README](https://github.com/includeos/demo-examples/blob/master/README.md) in the demo-examples repository.

##### Writing your first service

To start writing your first service,
- Copy the [demo_service/service.cpp](https://github.com/includeos/demo-examples/blob/master/demo_service/service.cpp) example.
- Then start implementing the `Service::start` function in the `Service` class, located in `<your-service-name>/service.cpp` (very simple example provided). This function will be called once the OS is up and running.
- Update the `CMakeLists.txt` to specify the name of your project, enable any needed drivers or plugins, etc.
- Update the `conanfile.txt` to specify any build requirements and generators.

You should then be able to run your service in the same way as the demo_service.

To add a new service to our demo examples, make a PR to our [demo-examples](https://github.com/includeos/demo-examples) repo. Make sure to
add a README in your example folder with description of your service.

**Note:** Remember to deactivate your service environment after your work with:

```
  $ source deactivate.sh
```

<!-- TODO: ### How building with Conan works? -->
___

### <a name="editing_includeos"></a> Getting IncludeOS in editable mode

To get started with getting the conan package in editable mode,

- ###### Edit the `conanfile.py` : comment out the `version = get_version()`

- ###### Create a `build` folder to build IncludeOS

- ###### Edit the `layout.txt` : _(uses jinja2 syntax)_ needed to parse the build folder correctly.

Do some local adaptions for where your build folder is by setting your build
folder in the first line as follows:

```
  {% set build_dir='build' %}
  [bindirs]
  cmake
  [includedirs]
  api
  [libdirs]
  {{ build_dir }}/plugins
  {{ build_dir }}/drivers
  {{ build_dir }}/lib
  [resdirs]
  {{ build_dir }}

```

- ###### Set the conan package into **editable mode**

Make sure to adjust the version to whatever is apropriate.

```
  conan editable add . includeos/0.15.0@includeos/test --layout=layout.txt
```
**Note:** Avoid choosing `latest`

- ###### Check Status

The package is now in **editable mode** and any dependencies of IncludeOS will
pick this IncludeOS package from your local cache. If you make any changes to the
code a simple `make` should be enough. However if your dependencies have changed
you need to redo the `conan install` step.

Here is an example on how it looks when its pulled into cache as editable:

```
  $ conan editable list
  includeos/0.15.0@includeos/test:45955af12a7f7608830c1d255b80a75440406e3c - Editable
```

- ###### Build IncludeOS

```
  cd build
  conan install .. -pr <conan_profile> (-o options like solo5=True etc)
  cmake .. (-DTOOLCHAIN_PROFILE=xx.yy when needed)
  make
```

- ###### Finalizing Changes

Once the code is **finalized** and you want to verify that the conan package
still builds remove the editable:

```
  $ conan editable remove includeos/0.15.0@includeos/test
```

Then remove the comment on the `#version` in the `conanfile.py` and do a normal

```
  $ conan create <source_path> includeos/test -pr <conan_profile>
```

___

### <a name="develop_pkg"></a> Getting started developing packages

Currently building works for `clang-6` and `gcc-7.3.0` compiler toolchains.
It is expected that these are already installed in your system.

##### Building Tools

The GNU Binutils are a collection of binary tools we have added to the profiles
so that the binaries are always executable inside the conan environment.
They are not an actual part of the final binary.

The binutils package must be built for the Host it's intended to run on. Therefore
the binutils package is built using a special _toolchain profile_ that doesn't have
a requirement on binutils itself.

To build `bintuils` using our [includeos/conan](https://github.com/includeos/conan/tree/master/dependencies/gnu/binutils/2.31) recipes:

- Clone the repository
- Do `conan create` as follows:

```
conan create <binutils-conan-recipe-path>/binutils/2.31 -pr <yourprofilename>-toolchain includeos/test
```

For MacOS users, we currently have a [apple-clang-10-macos-toolchain](https://github.com/includeos/conan_config/blob/master/profiles/apple-clang-10-macos-toolchain) for building a binutils package.

##### Building Dependencies

To build our other dependencies you may use the conan recipes we have in the
[conan](https://github.com/includeos/conan) repository.

**Note:** If you plan to build dependencies you might need to ensure you have
other missing libraries installed.

**Requirements**

- GNU C++ compiler - `g++-multilib`
- Secure Sockets Layer toolkit `libssl-dev`

###### Building musl
```
conan create <conan-recipe-path>/musl/1.1.18 -pr <yourprofilename> includeos/test
```

###### Building llvm stdc++ stdc++abi and libunwind

If these recipes do not have a fixed version in the conan recipe then you have
to specify it alongside the `user/channel` as `package/version@user/channel`
otherwise you can use the same format at musl above.

```
conan create <conan-recipe-path>/llvm/libunwind -pr <yourprofilename> libunwind/7.0.1@includeos/test
conan create <conan-recipe-path>/llvm/libcxxabi -pr <yourprofilename> libcxxabi/7.0.1@includeos/test
conan create <conan-recipe-path>/llvm/libcxx -pr <yourprofilename> libcxx/7.0.1@includeos/test
```
___

##### Building IncludeOS libraries and tools

We have moved the libraries and tools created by IncludeOS outside the IncludeOS
repository. You can now find them all in there own repositories inside the IncludeOS organization.

To build the libraries and tools,

```
  $ git clone https://github.com/includeos/mana.git
  $ cd mana
  $ conan create . includeos/test -pr clang-6.0-linux-x86_64
```

Below is a list of some of our Libraries/Tools:

- [Vmbuild](https://github.com/includeos/vmbuild) -
Vmbuild is an utility for building the IncludeOS virtual machines.

- [Vmrunner](https://github.com/includeos/vmrunner) -
Vmrunner is a utility developed for booting IncludeOS binanries.


- [Uplink](https://github.com/includeos/uplink) -
Uplink is a tool and a library for Live updating IncludeOS instances.

- [Mana](https://github.com/includeos/mana) -
Mana is a web application framework which is used to build a IncludeOS webserver.
We have a demo-example called [acorn](https://github.com/includeos/demo-examples/tree/master/acorn) which demonstrates a lot of potential.

- [Microlb](https://github.com/includeos/microlb) -
Microlb is a library written for building the IncludeOS load balancer.
We have a demo-example called [microlb](https://github.com/includeos/demo-examples/tree/master/microLB) which demonstrates our load balancer.

- [Mender](https://github.com/includeos/mender) -
Mender is a client for IncludeOS. We have a demo-example demonstrating
[mender](https://github.com/includeos/demo-examples/tree/master/mender) use case.

- [Diskbuilder](https://github.com/includeos/diskbuilder) -
Diskbuilder is a tool used for building disks for IncludeOS.


- [Chainloader](https://github.com/includeos/chainloader) -
Chainloader is a tool developed for building IncludeOS in x86 architectures.

- [NaCl](https://github.com/includeos/NaCl) -
NaCl is the configuration language tool we have tailored for IncludeOS to allow
users to configure various network settings such as firewall rules, vlans,
ip configurations etc.

___

## <a name="contribute"></a> Contributing to IncludeOS

IncludeOS is being developed on GitHub. Create your own fork, send us a pull request, and [chat with us on Slack](https://goo.gl/NXBVsc). Please read the [Guidelines for Contributing to IncludeOS](http://includeos.readthedocs.io/en/latest/Contributing-to-IncludeOS.html).

**Important: Send your pull requests to the `dev` branch**. It is ok if your pull requests come from your master branch.

## <a name="guideline"></a> C++ Guidelines

We want to adhere as much as possible to the [ISO C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines). When you find code in IncludeOS which doesn't adhere, please let us know in the [issue tracker](https://github.com/hioa-cs/IncludeOS/issues) - or even better, fix it in your own fork and send us a [pull-request](https://github.com/hioa-cs/IncludeOS/pulls).

[brew]: https://brew.sh/
[qemu]: https://www.qemu.org/

## <a name="security"></a> Security contact
If you discover a security issue in IncludeOS please avoid the public issue tracker. Instead send an email to security@includeos.org. For more information and encryption please refer to the [documentation](http://includeos.readthedocs.io/en/latest/Security.html).
