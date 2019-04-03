![IncludeOS Logo](./etc/logo.png)
================================================

**IncludeOS** is an includable, minimal [unikernel](https://en.wikipedia.org/wiki/Unikernel) operating system for C++ services running in the cloud. Starting a program with `#include <os>` will literally include a tiny operating system into your service during link-time.

IncludeOS is free software, with "no warranties or restrictions of any kind".

[![Pre-release](https://img.shields.io/github/release-pre/includeos/IncludeOS.svg)](https://github.com/hioa-cs/IncludeOS/releases)
[![Apache v2.0](https://img.shields.io/badge/license-Apache%20v2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)
[![Join the chat](https://img.shields.io/badge/chat-on%20Slack-brightgreen.svg)](https://goo.gl/NXBVsc)

**Note:** *IncludeOS is under active development. The public API should not be considered stable.*

## Quick start
### Install the dependencies
On macOS there is a work in progress homebrew package here: https://github.com/includeos/homebrew-includeos
#### Building IncludeOS services:
* [The conan package manager](https://docs.conan.io/en/latest/installation.html)
* cmake, make, nasm
* clang, or alternatively gcc on linux

#### Running an IncludeOS service (using boot):
* qemu
* python3
* python packages: psutil, jsonschema

Now install the conan profiles and remotes for getting binary packages of includeos libraries.
```
$ conan config install https://github.com/includeos/conan_config.git
```

### Hello world with IncludeOS
First select an appropriate [conan profile](https://docs.conan.io/en/latest/reference/profiles.html) for the target you want to boot on. `conan profile list` will show the profiles available, including the ones installed in the previous step. When developing for the machine you're currently on, linux users can typically use `clang-6.0-linux-x86_64`, and MacOS users can use `clang-6.0-macos-x86_64`. You can also make your own.

The following steps let you build and boot the IncludeOS hello world example.
```
$ git clone https://github.com/includeos/hello_world.git
$ mkdir your_build_dir
$ cd your_build_dir
$ conan install ../hello_world -pr <your_conan_profile>
$ source ./activate.sh
$ cmake ../hello_world
$ cmake --build .
$ boot hello
```
You can use the [hello world repo](https://github.com/includeos/hello_world) as a starting point for developing your own IncludeOS services. For more advanced examples see the [examples repo](https://github.com/includeos/demo-examples) or the integration tests (under ./IncludeOS/test/\*/integration).

Once you're done `$ source ./deactivate.sh` will reset the environment to its previous state.

### IncludeOS kernel development
The above shows how to build bootable IncludeOS binaries using pre-built versions the IncludeOS libraries. In order to modify IncludeOS itself or contribute to IncludeOS development, you need a different workflow. Start by cloning IncludeOS.

```
$ git clone https://github.com/includeos/IncludeOS.git
```

Now create a conan layout file, or edit your own copy of [layout.txt](https://github.com/includeos/IncludeOS/blob/dev/etc/layout.txt). Have the first line point to `your_kernel_build_dir`, e.g. where you want the compiled IncludeOS libraries to be built while you work. You can now tell conan to use this local build instead of the prebuilt binary versions.

```
$ conan editable add your_IncludeOS_fork -l your_layout.txt includeos/0.15.0@includeos/test
$ cd your_kernel_build_dir
$ conan install your_IncludeOS_fork -pr <your conan profile>
$ conan build your_IncludeOS_fork -sf your_IncludeOS_fork -bf .
```

Building a service that depends on `includeos <=0.15` will now depend on the local copy of the IncludeOS kernel libraries. You can now make changes to the IncludeOS source (pointed to by the `conan build ... -sf `parameter) and simply
```
$ cd your_kernel_build_dir
$ cmake --build . # or simply make, if you use the default cmake backend
```
Test your changes e.g. in the hello world example like so:
```
$ cd your_build_dir
$ rm bin/hello.elf.bin # force cmake to relink
$ source ./activate.sh
$ cmake ../hello_world
$ cmake --build .
$ boot hello
```

##### Contents:

- [Key Features](#features)
- [Getting started with IncludeOS development](#getting_started)

- ###### Service Developers Howto:
  * [Building IncludeOS with dependencies from conan](#building_includeos)
  * [Building, starting and creating your first IncludeOS Service](#building_service)

- ###### Kernel Developers Howto:
  * [Getting IncludeOS in Editable mode](#editing_includeos)
  * [Getting started developing packages](#develop_pkg)

- ###### Other Info:
  * [Contributing to IncludeOS](#contribute)
  * [Our Website](https://www.includeos.org/)
  * [The Company](https://www.includeos.com/)
  * [C++ Guidelines](#guideline)
  * [Security contact](#security)


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
    $ git clone https://github.com/includeos/IncludeOS
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
    $ conan search help
```

To access all the available packages on our includeos remote:

```
    $ conan search -r includeos
```

This should list all the packages we have uploaded to the includeos repository.
We allow uploading of packages to two channels, namely:
- `stable`: this channel has all the stable packages.
- `latest`: this channel will have the latest package
  in development/test phase.

To find all the stable versions uploaded for a particular package, try:

```
    $ conan search -r includeos '<package_name>/*@includeos/stable'
```
> **NOTE:** We only guarantee that the **latest 10 packages** are kept in the
`latest` channel. All `stable` packages will be kept in the stable channel unless
proven unsafe. One suggested workaround is to copy packages into your own repository.
YOu can do that using the copy command:

```
    $ conan copy --all -p <path-to-conanfile> <package_name>@<your_repository>/<your_channel>
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
  $ source activate.sh
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
If you skipped the step of activating the virtual environment in the previous step remeber to do:

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

To build more examples follow the [README](https://github.com/includeos/demo-examples/blob/master/README.md) in the examples repository.

##### Writing your first service

To start writing your first service,
- Copy the [demo_service/service.cpp](https://github.com/includeos/demo-examples/blob/master/demo_service/service.cpp) example.
- Then start implementing the `Service::start` function in the `Service` class, located in `<your-service-name>/service.cpp` (very simple example provided). This function will be called once the OS is up and running.
- Update the `CMakeLists.txt` to specify the name of your project, enable any needed drivers or plugins, etc.
- Update the `conanfile.txt` to specify any build requirements and generators.

You should then be able to run your service in the same way as the demo_service.

To add a new service to our demo examples, make a PR to our [demo-examples](https://github.com/includeos/demo-examples) repo. Make sure to
add a README in your example folder with description of your service.


> **Note:** Remember to deactivate your service environment after your work with:

```
  $ source deactivate.sh
```

<!-- TODO: ### How building with Conan works? -->
___

### <a name="editing_includeos"></a> Getting IncludeOS in Editable mode

If you are a kernel developer or are simple interested in fiddling with our
kernel code, you can use the includeos package in `editable` mode. This is useful
when working with several interconnected components and you would like to test
your changes across several libraries or functionalities.

Below we have written down a few steps to get you started with editable packages.
You can read more about it at [packages in editable mode](https://docs.conan.io/en/latest/developing_packages/editable_packages.html).

> **Note:** Currently this is an experimental feature on conan version 1.13 and they
have mentioned that for future releases the feature is subject to breaking changes.

To get started with getting the conan package in editable mode,

- ###### Create a `build` folder to build IncludeOS

- ###### Edit the `etc/layout.txt` : _(uses jinja2 syntax)_ needed to parse the build folder correctly.

Do some local adaptions for where your build folder is relative to the includeos source folder by setting build_dir in the third line as follows :
```
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


- ###### Set the conan package into **editable mode**

Make sure to adjust the version to whatever is apropriate.

```
  conan editable add . includeos/$(conan inspect -a version . | cut -d " " -f 2)@includeos/latest --layout=etc/layout.txt
```

- ###### Check Status

The package is now in **editable mode** and any dependencies of IncludeOS will
pick this IncludeOS package from your local cache. If you make any changes to the
code a simple `make` should be enough. However if your dependencies have changed
you need to redo the `conan install` step.

Here is an example on how it looks when its pulled into cache as editable:

```
  $ conan editable list
    includeos/0.14.1-1052@includeos/latest
      Path: ~/git/IncludeOS
      Layout: ~/git/IncludeOS/etc/layout.txt
```

- ###### Build IncludeOS
Asuming the buildfolder is build under the includeos source directory the following is enough.
you can also manually perform the build step for the editable package however doing the step below ensures all parameters are transfered correctly from your conan profile and options into the build.
```
  conan install -if build . -pr <conan_profile> (-o options like platform=nano etc)
  conan build -bf build .
```
- ###### building small changes once the first build is done
Once IncludeOS is buildt by the conan build command you simply have to make sure to issue a make command in the build location
```
  cd build && make
  or
  cmake build --build
```
- ###### Finalizing Changes

Once the code is **finalized** and you want to verify that the conan package
still builds remove the editable and do a conan create on the package:

```
  $ conan editable remove includeos/0.15.0@includeos/test
  $ conan create <source_path> includeos/Latest -pr <conan_profile>
```

___

### <a name="develop_pkg"></a> Getting started developing packages

If you are interested in editing/building our dependency packages on your own,
you can checkout our repositories at [includeos/](https://github.com/includeos/).
Some our tools and libraries are listed below under [Tools and Libraries](#libs_tools).
You can find the external dependency recipes at [includeos/conan](https://github.com/includeos/conan).
Currently we build with `clang-6` and `gcc-7.3.0` compiler toolchains.
It is expected that these are already installed on your system.

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

> **Note:** If you plan to build dependencies you might need to ensure you have
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
repository. You can now find them all in their own repositories inside the IncludeOS organization.

To build the libraries and tools,

```
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
NaCl is the configuration language tool we have tailored for IncludeOS to allow
users to configure various network settings such as firewall rules, vlans,
ip configurations etc.

___

## <a name="contribute"></a> Contributing to IncludeOS

IncludeOS is being developed on GitHub. Create your own fork, send us a pull request, and [chat with us on Slack](https://goo.gl/NXBVsc). Please read the [Guidelines for Contributing to IncludeOS](http://includeos.readthedocs.io/en/latest/Contributing-to-IncludeOS.html).

> **Important: Send your pull requests to the `dev` branch**. It is ok if your pull requests come from your master branch.

## <a name="guideline"></a> C++ Guidelines

We want to adhere as much as possible to the [ISO C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines). When you find code in IncludeOS which doesn't adhere, please let us know in the [issue tracker](https://github.com/includeos/IncludeOS/issues) - or even better, fix it in your own fork and send us a [pull-request](https://github.com/includeos/IncludeOS/pulls).

[brew]: https://brew.sh/
[qemu]: https://www.qemu.org/

## <a name="security"></a> Security contact
If you discover a security issue in IncludeOS please avoid the public issue tracker. Instead send an email to security@includeos.org. For more information and encryption please refer to the [documentation](http://includeos.readthedocs.io/en/latest/Security.html).
