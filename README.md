# mender_client

mender.io client for IncludeOS.

## Prerequisites
* [Botan](https://github.com/randombit/botan) (compiled for IncludeOS)
* LiveUpdate

Both these are prebuilt and supplied in [deps](./deps).


## Installation

### Script

```
export $INCLUDEOS_PREFIX=<my-installation> # defaults to /usr/local
./install.sh
```

This will install Botan and LiveUpdate in your IncludeOS installation (`$INCLUDEOS_PREFIX/includeos`) and run cmake and install the mender client at the same location.

### Manual

Make sure Botan and LiveUpdate is installed. If these are installed somewhere else than in your IncludeOS installation, make sure to update the [CMakeLists.txt](./CMakeLists.txt) with the correct paths (`LOCAL_INCLUDES` and `LIBRARIES`, both when building library and services).

```
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=<your-includeos-installation> # -Dverbose=ON
make install
```

## Usage

See the [example](./example) on how to build a service using the client.

### Mender backend

Clone the [mender/integration](https://github.com/mendersoftware/integration/tree/1.0.x) (**v1.0.x** branch). Patch it to run without TLS (see [discussion](https://groups.google.com/a/lists.mender.io/forum/#!topic/mender/9pwno7eoGKE)). [Download patch](https://groups.google.com/a/lists.mender.io/group/mender/attach/a242fd1cf6601/0001-no-ssl-export-minio-on-non-SSL-port.patch?part=0.1&authuser=1). 