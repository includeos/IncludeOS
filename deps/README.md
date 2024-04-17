# IncludeOS dependencies
Build scripts and tools for all IncludeOS dependencies. The maintained build system is [Conan](https://conan.io/), currently supporting version `2.2.2`.

## Toolchain
The toolchain used in active development are release binaries from LLVM installed with their script at [https://apt.llvm.org/](https://apt.llvm.org/) on Ubuntu 22.04.

```
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18
```

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