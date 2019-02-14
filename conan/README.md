# Conan
Getting started with conan building Packages (usage will be covered later once we start using it)

The IncludeOS conan recepies are developed for conan [1.8.4](https://github.com/conan-io/conan/releases/tag/1.8.4) or newer.
The [documentation](https://docs.conan.io/en/latest/index.html) is a good reference for further reading

# Getting started using conan for IncludeOS development
Profiles can be found in conan/profiles folder in the IncludeOS repository and installed by copying them to ~/.conan/profiles. In the future we hope to install them with
[conan config install](https://docs.conan.io/en/latest/reference/commands/consumer/config.html#conan-config-install)

Add the IncludeOS-Develop conan Artifactory repo
```
conan remote add includeos-test https://api.bintray.com/conan/includeos/test-packages
```
The following dependencies are required when building:
```
apt install cmake gcc-7 g++-multilib clang-6.0 libssl-dev lcov
```

# Building includeos with dependencies from conan
Build includeos using clang-6.0. if no CONAN_PROFILE is defined it will build using clang-6.0 by default.
Passing ́`-DCONAN_DISABLE_CHECK_COMPILER` disables this check
If the environment variable `INCLUDEOS_PREFIX` is set the install will install includeos almost like before.
```
cmake -DCONAN_PROFILE=clang-6.0-linux-x86_64 <path to includeos repo>
make
make install
```

# Searching Packages
if you want to check if a package exists you can search for it
`conan search help`

# Getting started developing packages
## profile and depencencies
Currently building works for clang-6 and gcc-7.3.0 compiler toolchain. It is expected that these are already installed in your system. However we hope to provide toolchains in the future.

The target profiles we have verified are the following:
- [clang-6.0-linux-x86](profiles/clang-6.0-linux-x86)
- [clang-6.0-linux-x86_64](profiles/clang-6.0-linux-x86_64)
- [gcc-7.3.0-linux-x86_64](profiles/gcc-7.3.0-linux-x86_64)
- [clang-6.0-macos-x86_64](profiles/clang-6.0-macos-x86_64)

Copy these to your `~/.conan/profiles` folder to make them available to conan. An `x86` profile is simply replacing the `arch=x86_64` arch to `arch=x86`. The toolchain version would be the same profile but without the `[build_requres]` section.

You can see your list of profiles
```
conan profile list
```
view the content by
```
conan profile show <yourprofilename>
```
## building tools
Binutils is a tool and not an actual part of the final binary by having it added in the profile the binaries are allway executable inside the conan env.

The binutils tool must be buildt for the Host it's intended to run on. Therfore the binutils package is buildt using a special toolchain profile that doesnt have a requirement on binutils.

```
conan create <includeos/conan>/binutils/2.31 -pr <yourprofilename>-toolchain includeos/test
```

##building musl
```
conan create <includeos/conan>/musl/v1.1.18 -pr <yourprofilename> includeos/test
```

## building llvm stdc++ stdc++abi and libunwind
These recipes do not have a fixed version in the recipe so you have to specify it alongside `user/channel` so its `package/version@user/channel`
```
conan create <includeos/conan>/llvm/libunwind -pr <yourprofilename> libunwind/7.0@includeos/test
conan create <includeos/conan>/llvm/libcxxabi -pr <yourprofilename> libcxxabi/7.0@includeos/test
conan create <includeos/conan>/llvm/libcxx -pr <yourprofilename> libcxx/7.0@includeos/test

```

## test deploy a package or a package with all the dependencies
```
conan install package/version@user/channel -pr <yourprofilename>
```
