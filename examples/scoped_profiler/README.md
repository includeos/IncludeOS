### IncludeOS Scoped Profiler Demo

Add `ScopedProfiler sp;` in the scopes that you want to profile. Don't forget to `#include <profile>`.

Rebuild / install IncludeOS:

```
cd IncludeOS/src
make
make install
```

Build and run this service:

```
cd IncludeOS/examples/scoped_profiler
make
./run.sh
```

Make something happen in the OS and then use wget or curl to `GET /profile` to see statistics:

```
curl 10.0.0.42/profile
```
