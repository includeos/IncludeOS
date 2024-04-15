# IncludeOS dependencies
Build scripts and tools for all IncludeOS dependencies. The maintained build system is [Conan](https://conan.io/), currently supporting version `2.2.2`.

## Conanfiles for IncludeOS dependencies
The current conanfiles were ported from Conan 1.x versions in `https://github.com/includeos/conan`.

To build all IncludeOS dependencies, for each dependency, do `conan create .` in each directory next to the `conanfile.py`. (This is what `build_all.sh` will do, but it's very WIP). This will populate your local conan cache with the built packages.

## Conan profile
This is the conan profile currently used in active development:

```
[settings]
arch=x86_64
build_type=Release
compiler=clang
compiler.cppstd=17
compiler.libcxx=libc++
compiler.version=14
os=Linux
[buildenv]
CC=/usr/bin/clang
CXX=/usr/bin/clang++
```

## Old build scripts
The original bash build scripts are no longer maintained, but might provide useful context. They can be found in the `v0.14.1` release tree, [https://github.com/includeos/IncludeOS/tree/v0.14.1/etc](https://github.com/includeos/IncludeOS/tree/v0.14.1/etc). There are scripts for each of the dependencies, called from `create_binary_bundle.sh`. 