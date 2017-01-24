
# Demo Service

### What
Serves a static webpage at address `10.0.0.42` on port 80

Demonstrates a very basic tcp server.  
For a better example look at [acorn](https://github.com/hioa-cs/IncludeOS/tree/dev/examples/acorn)

---
### Building
Make sure IncludeOS is properly installed and the environment variables are set.

Navigate to `IncludeOS/examples/demo_example/build`  
Build using your cmake specified build system

eg: `make`

---
### Running
Navigate to `IncludeOS/examples/demo_example/build`  
Call the `run.sh` script and pass the Image as argument

eg: `../run.sh IncludeOS_example`
