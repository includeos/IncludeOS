### IncludeOS demo in Linux Userspace

Build and install the linux userspace library first, from the linux folder:
```
mkdir -p build && pushd build && cmake .. -DCMAKE_INSTALL_PREFIX=$INCLUDEOS_PREFIX && make -j4 install && popd
```

Build with cmake & make, then run:
```
mkdir -p build && pushd build && cmake .. && make && popd
./build/demo_example
```

This demo-service should start an instance of IncludeOS that brings up a minimal web service on port 80 with static content.

The default static IP is 10.0.0.2.
