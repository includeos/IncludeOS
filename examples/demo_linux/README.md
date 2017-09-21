### IncludeOS Demo Service

Build with cmake & make, then run:
```
mkdir -p build && pushd build && cmake .. && make && popd
./build/demo_example
```

This demo-service should start an instance of IncludeOS that brings up a minimal web service on port 80 with static content.

The default static IP is 10.0.0.2.
