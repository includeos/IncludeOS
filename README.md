IncludeOS
================================================

**IncludeOS** is an includable, minimal [unikernel](https://en.wikipedia.org/wiki/Unikernel) operating system for C++ services running in the cloud and on real HW. Starting a program with `#include <os>` will literally include a tiny operating system into your service during link-time.

IncludeOS is free software, with "no warranties or restrictions of any kind".

**Note:** *IncludeOS is under active development. The public API should not be considered stable.*

## <a name="features"></a> Key features

* **Extreme memory footprint**: A minimal bootable 64-bit web server, including operating system components and anything needed from the C/C++ standard libraries is currently 2.5 MB.
* **KVM, VirtualBox and VMWare support** with full virtualization, using [x86 hardware virtualization](https://en.wikipedia.org/wiki/X86_virtualization), available on most modern x86 CPUs. IncludeOS will run on any x86 hardware platform, even on a physical x86 computer, given appropriate drivers. Officially, we develop for- and test on [Linux KVM](http://www.linux-kvm.org/page/Main_Page), and VMWare [ESXi](https://www.vmware.com/products/esxi-and-esx.html)/[Fusion](https://www.vmware.com/products/fusion.html) which means that you can run your IncludeOS service on Linux, Microsoft Windows and macOS, as well as on cloud providers such as [Google Compute Engine](http://www.includeos.org/blog/2017/includeos-on-google-compute-engine.html), [OpenStack](https://www.openstack.org/) and VMWare [vcloud](https://www.vmware.com/products/vcloud-suite.html).
* **Instant boot:** IncludeOS on Qemu/kvm boots in about 300ms but IBM Research has also integrated IncludeOS with [Solo5/uKVM](https://github.com/Solo5/solo5), providing boot times as low as 10 milliseconds.
* **Modern C++ support**
    * Full C++11/14/17/20 language support with [clang](http://clang.llvm.org) 18 and later.
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
    - [Kernel development](#develop_kernel)
    - [Running tests](#running_tests)
- [Contributing to IncludeOS](#contribute)
- [C++ Guidelines](#guideline)
- [Security contact](#security)

## <a name="getting_started"></a> Getting started

### <a name="dependencies"></a> Dependencies

For building and booting IncludeOS services you will need [nix](https://nixos.org) and Linux. Nix will automatically download and set up the correct versions of all the required libraries and compilers.

To speed up local builds we also recommend configuring nix with [ccache support](https://nixos.wiki/wiki/CCache) but this is not a requirement. To use ccache, `--arg withCcache true` can be added to most `nix-build` and `nix-shell` commands shown below.

IncludeOS can currently not be built on macOS or Windows.

### <a href="hello_world"></a> Hello World

A minimal IncludeOS "hello world" looks like a regular C++ program:

```c++
#include <iostream>

int main(){
  std::cout << "Hello world\n";
}
```

A full "Hello world" service with a working nix workflow is available in the [hello world repo](https://github.com/includeos/hello_world). The repository can also be used as a a starting point for developing your own IncludeOS service.

For more advanced service examples see the the integration tests (under ./IncludeOS/test/\*/integration).

### <a name="develop_kernel"></a> Kernel development

To build IncludeOS, run

```bash
$ nix-build
```

This will build the toolchain and all IncludeOS kernel libraries.

Note that the first build will take some time to complete, as the IncludeOS toolchain is rebuilt from source code. This includes clang, llvm, libcxx, musl and so on. There is no nix binary cache available for these files at the moment. Subsequent builds will go much faster when the toolchain has been cached in the local nix-store.

After making changes to the kernel, run `nix-build` again to get new binaries. If you are iterating on changes in one section of the kernel you can speed up the build significantly by using ccache. All `nix-build` and `nix-shell` commands in this section support the optional parameter `--arg withCcache true`.

It's not always practical to rebuild the whole kernel during development. You can get a development shell with a preconfigured environment using `shell.nix`:

```bash
$ nix-shell
```

Further instructions will be shown for optionally configuring VM networking or overriding the build path when starting the shell.

By default th shell will also build the unikernel from `example.nix`. The example unikernel can be booted from within the shell:

```bash
$ nix-shell
[...]
nix$ boot hello_includeos.elf.bin
```

If you want to build a different unikernel than the example, this can be specified with the `--argstr unikernel [path]` parameter. This is primarily used for integration tests. For example, to build and run the stacktrace-test:

```bash
$ nix-shell --argstr unikernel ./test/kernel/integration/stacktrace
[...]
nix$ ls -l kernel*
kernel_stacktrace
nix$ boot kernel_stacktrace
[...]
Calling os::print_backtrace()
symtab or strtab is empty, indicating image may be stripped
[0] 0x000000000025dcd2 + 0x000: 0x25dcd2
[1] 0x000000000021097d + 0x000: 0x21097d
[2] 0x00000000002b370a + 0x000: 0x2b370a
[3] 0x0000000000210eea + 0x000: 0x210eea
We reached the end.
```

To build and run the test VM as a single command:

```bash
$ nix-shell --argstr unikernel ./test/kernel/integration/stacktrace --run ./test.py
```

### <a name="running_tests"></a> Running tests

You can run all the integration tests using the script `./test.sh`. The tests will run locally in the nix environment. We recommend manually verifying that all the tests pass locally before submitting a new PR to IncludeOS to save review time.

Individual tests can be run with `nix-shell` directly. See `test.sh` for more details.

## <a name="contribute"></a> Contributing to IncludeOS

IncludeOS is being developed on GitHub. Create your own fork and send us a pull request. Please read the [Guidelines for Contributing to IncludeOS](http://includeos.readthedocs.io/en/latest/Contributing-to-IncludeOS.html).

## <a name="guideline"></a> C++ Guidelines

We want to adhere as much as possible to the [ISO C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines). When you find code in IncludeOS which doesn't adhere, please let us know in the [issue tracker](https://github.com/includeos/IncludeOS/issues) - or even better, fix it in your own fork and send us a [pull-request](https://github.com/includeos/IncludeOS/pulls).

## <a name="security"></a> Security contact
If you discover a security issue in IncludeOS please avoid the public issue tracker. Instead send an email to security@includeos.org. For more information and encryption please refer to the [documentation](http://includeos.readthedocs.io/en/latest/Security.html).
