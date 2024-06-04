# IncludeOS dependencies
Build scripts and tools for all IncludeOS dependencies. The maintained build system is [Conan](https://conan.io/), currently supporting version `2.2.2`. Submissions for other build scripts and build systems for the dependencies are welcome as long as they are open source and easy to test for most user with the right hardware.

## Toolchain
The toolchain used in active development are LLVM binaries from apt, provided by apt.llvm.org, on Ubuntu 22.04.

```
sudo apt install clang-18 libc++-18-dev libc++abi-18-dev
```

In addition you'll need cmake, as well as python3 and pip to install conan.

## Conanfiles for IncludeOS dependencies
The current conanfiles were ported from Conan 1.x versions in `https://github.com/includeos/conan`.

To build all IncludeOS dependencies, for each dependency, do `conan create .` in each directory next to the `conanfile.py`. (This is what `build_all.sh` will do, but it's very WIP). This will populate your local conan cache with the built packages.

Other compilers and distributions might work just fine, but the llvm scripts installs some libraries that e.g. the ubuntu distribution doesn't ship by default such as libusan, currently required to build openssl. 


## Conan profile
This is the conan profile currently used in active development:

```
[settings]
arch=x86_64
build_type=Release
compiler=clang
compiler.cppstd=17
compiler.libcxx=libc++
compiler.version=18
os=Linux

[buildenv]
CC=/usr/bin/clang-18
CXX=/usr/bin/clang++-18
```

It can be added to `~/.conan2/prfiles/clang18` and used like this: `conan create -pr clang18 .`

## Old build scripts
The original bash build scripts are no longer maintained, but might provide useful context. They can be found in the `v0.14.1` release tree, [https://github.com/includeos/IncludeOS/tree/v0.14.1/etc](https://github.com/includeos/IncludeOS/tree/v0.14.1/etc). There are scripts for each of the dependencies, called from `create_binary_bundle.sh`. 