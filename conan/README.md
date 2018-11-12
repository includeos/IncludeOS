# Conan
Getting started with conan building Packages (usage will be covered later once we start using it)

The IncludeOS conan recepies are developed for conan [1.8.4](https://github.com/conan-io/conan/releases/tag/1.8.4).
The [documentation](https://docs.conan.io/en/latest/index.html) is a good reference for further reading

# Install latest stable
create the IncludeOS default clang-5.0 profile file in ~/.conan/profiles/clang-5.0
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
conan remote add includeos http://mothership.includeos.org:8090/artifactory/api/conan/conan-local
```

Download the current stable precompiled libraries to the current folder
```
conan install includeos-precompiled/0.13.0@includeos/stable -pr clang-5.0
```

Build includeos with these Packages in current directory
```
cmake -DBUNDLE_LOC=<PathToCurrentFolder>/PrecompiledLibraries.tgz <PathToIncludeOS_GIT>
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
