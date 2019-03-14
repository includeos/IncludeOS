![IncludeOS Logo](./doc/logo.png)
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

### Key features

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

### Getting started with IncludeOS development

The [IncludeOS](https://www.includeos.org/) conan recipes are developed with [Conan version 1.12.3](https://github.com/conan-io/conan/releases/tag/1.12.3) or newer.

For Mac OS ensure that you have a working installation of [brew](https://brew.sh/) to be able to install all dependencies.


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

### Building IncludeOS with dependencies from conan

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
conan remote add includeos-test https://api.bintray.com/conan/includeos/test-packages
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
are in progress._ <!-- needs update -->


###### Searching Packages

if you want to check if a package exists you can search for it:

```
    conan search help
```

___

<!-- TODO: How building with Conan works? -->

### Getting started developing packages

Currently building works for `clang-6` and `gcc-7.3.0` compiler toolchain.
It is expected that these are already installed in your system. However we
hope to provide toolchains in the future.

##### Building Tools

Binutils is a tool and not an actual part of the final binary by having it
added in the profile the binaries are always executable inside the conan
environment.

The binutils tool must be built for the Host it's intended to run on. Therefore
the binutils package is built using a special toolchain profile that doesn't have
a requirement on binutils.

To build `bintuils` using our [includeos/conan](https://github.com/includeos/conan/tree/master/dependencies/gnu/binutils/2.31) recipes:

- Clone the repository
- Do `conan create` as follows:

```
conan create <binutils-conan-recipe-path>/binutils/2.31 -pr <yourprofilename>-toolchain includeos/test
```
##### Building Dependencies

To build our other dependencies you may use the conan recipes we have in the
repository.

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

<!-- TODO: How to build examples with conan? -->

<!-- TODO: How to build libraries with conan? -->

<!-- TODO: How to build services and plugins with conan? -->
___

### Testing the IncludeOS build

<!-- TODO: needs update on how to build on conan -->
___

### Writing your first service

<!-- TODO: needs update on how to build on conan -->

___

## Contributing to IncludeOS

IncludeOS is being developed on GitHub. Create your own fork, send us a pull request, and [chat with us on Slack](https://goo.gl/NXBVsc). Please read the [Guidelines for Contributing to IncludeOS](http://includeos.readthedocs.io/en/latest/Contributing-to-IncludeOS.html).

**Important: Send your pull requests to the `dev` branch**. It is ok if your pull requests come from your master branch.

<!-- TODO: More on Contributing using conan -->

## C++ Guidelines

We want to adhere as much as possible to the [ISO C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines). When you find code in IncludeOS which doesn't adhere, please let us know in the [issue tracker](https://github.com/hioa-cs/IncludeOS/issues) - or even better, fix it in your own fork and send us a [pull-request](https://github.com/hioa-cs/IncludeOS/pulls).

[brew]: https://brew.sh/
[qemu]: https://www.qemu.org/

## Security contact
If you discover a security issue in IncludeOS please avoid the public issue tracker. Instead send an email to security@includeos.org. For more information and encryption please refer to the [documentation](http://includeos.readthedocs.io/en/latest/Security.html).
