# IncludeOS POSIX extensions

IncludeOS intends to provide a POSIX interface suffucient for linking and running many conventional C libraries and programs. A lot of the POSIX functionality will be header-only stubs and some of it is provided externally by e.g. the compiler or standard library.

### Other providers of POSIX content
* *newlib*: is our current C library which also provides many POSIX features (indeed the C standard itself overlaps with POSIX). Newlib provides most of the C standard library, including `stdlib.h`, `stdio.h`, `math.h` etc., but is mising some C11 extensions. Those are rarely used and  provided here as stubs.
* *clang*: Clang provides a few POSIX headers such as `stddef.h`, `stdarg.h` and `limits.h`. It also provides compiler intrinsics such as `x86intrin.h`. When building IncludeOS we use the `-nostdlibinc` flag to allow inclusion of these headers, without including the standard library headers from the host.

### Guidelines for this folder
* Only actually standardized POSIX content should live here, and only content not allready provided by alternative sources above.
* Extensions to POSIX headers that IncludeOS needs, but which isn't present on one of the supportet platforms (e.g. macOS or Linux) should not live here, since we'd like to be able to build directly on those platforms with their respective POSIX implementations. As an example, our syslog implementation defines `LOG_INTERNAL` in addition to `LOG_MAIL` etc. While defining this symbol in the `syslog.h` POSIX header is allowed by the standard it introduces an implicit expectation in IncludeOS application code making it less portable. Such extensions can be placed in the IncludeOS API instead.
