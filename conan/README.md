# Conan
Getting started with conan building Packages (usage will be covered later once we start using it)

The IncludeOS conan recepies are developed for conan [1.8.4](https://github.com/conan-io/conan/releases/tag/1.8.4) or newer.
The [documentation](https://docs.conan.io/en/latest/index.html) is a good reference for further reading

# Getting started using conan for includeos development
Profiles can be found in conan/settings folder in the includeos repository and installed with
[conan config install](https://docs.conan.io/en/latest/reference/commands/consumer/config.html#conan-config-install)
example settings file
```
[settings]
os=Linux
os_build=Linux
arch=x86_64
arch_build=x86_64
compiler=clang
compiler.version=5.0
compiler.libcxx=libstdc++11
build_type=Release
[options]
[build_requires]
[env]
CC=clang-5.0
CXX=clang++-5.0
```
Add the includeos artifactory conan repo
```
conan remote add includeos https://includeos.jfrog.io/includeos/api/conan/conan-local
```

Build includeos using clang-5.0. if no CONAN_PROFILE is defined it will build using clang-5.0 by default. PS you must also set your host CC and CXX to clang-5.0 for the time beeing
```
cmake -DCONAN_PROFILE=clang-5.0 <path to includeos repo>
```

# Getting started developing
## profile and depencencies
Currently building the libraries using conan only works using clang5 and clang6 compiler toolchain. its expected that these are already installed in your systems
in your ~/.conan/profiles folder create for instance a clang-6.0 profile that looks something like
```
[settings]
os=Linux
os_build=Linux
arch=x86_64
arch_build=x86_64
compiler=clang
compiler.version=6.0
compiler.libcxx=libstdc++
build_type=Release
[options]
[build_requires]
[env]
CC=clang-6.0
CXX=clang++-6.0

```
you can se the profile by writing
        `conan profile list`
view the content by typing
        `conan profile show <yourprofilename>`
## building binutils,musl
In order to build must you first need to build binutils the musl package depends on it and will fail with an apropriate warning if you try on its own
```
conan create /path/to/includeos/conan/binutils/2.31 -pr <yourprofilename> includeos/test
conan create /path/to/includeos/conan/musl/v1.1.18 -pr <yourprofilename> includeos/test
```

## building llvm stdc++ stdc++abi and libunwind
```
conan create /path/to/includeos/conan/llvm/7.0 -pr <yourprofilename> includeos/test
```

## test deploy binutils musl and llvm to a local folder
```
conan install binutils/2.31@includeos/test
conan install musl/v1.1.18@includeos/test
conan install llvm/7.0@includeos/test
```
