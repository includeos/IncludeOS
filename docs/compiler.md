Using your own compiler
==========

A common ... is to use your own compiler, from trunk.

To do that, there are some steps you may have to do.

Copy one of the `-toolchain` profiles that already exists for your architecture, edit the compiler.version and the CC/CXX variables. They are stored in the `.conan/profile` directory on Linux. Now we need to build binutils only for this profile. Clone the `includeos/conan` repository, and enter `dependencies/gnu/binutils`. Now we can build binutils with the new toolchain profile like this:

```
conan create -pr gcc-9.1.0-linux-x86_64-toolchain . binutils/2.31@includeos/stable
```
NOTE: the order of arguments matter here. If you put the dot (.) before the -pr argument you will get an unhelpful error.

Also, if you don't know the specific package name you can see it in an error message when you try to build the OS with your compiler profile.

The binutils package should have been built now, which opens up the possibility for building dependencies for IncludeOS and the OS itself. Make a copy of a normal build profile with the same architecture and compiler (not a toolchain profile) in the conan profiles directory.

```
cp gcc-7.3.0-linux-x86_64 gcc-9.1.0-linux-x86_64
```
As usual, update compiler.version and the CC/CXX flags with the new values. `compiler.version=9`, `CC=gcc-9`, `CXX=g++-9`.

Now go into the IncludeOS repo, create a build folder, enter it and we will build the dependencies and then the os. Example:
```
conan install .. -pr gcc-9.1.0-linux-x86_64 --build
```
The --build argument forces a build from source. If our recipes work, it should now start building all the dependencies.

If the build succeeds we should have a few files in the build folder now:
```
activate.sh
conanbuildinfo.cmake
conanbuildinfo.txt
conaninfo.txt
conan.lock
deactivate.sh
environment.sh.env
graph_info.json
```
Type `source activate.sh` to activate the build environment needed by CMake. Your terminal line should now start with (conanenv).

Now build the OS like normal:
```
cmake ..
make -j16
```

If you have any strange build issues with things that seem like system includes, that is, includes that originate from /usr/include and paths obviously in your system, then that is a problem as they should be disabled. Post an issue or extend the CC/CXX flags with `-nostdinc` and friends.
