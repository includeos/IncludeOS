# Syslog IncludeOS Plugin Service

Sending syslog data over UDP port 6514. The message format corresponds to the format specified in RFC5424.

The default behavior for IncludeOS's syslog implementation is to send the data to printf, which again can be routed anywhere the service specifies. This is intentional since logging over a UDP/UNIX socket wouldn't necessarily work as one might think in IncludeOS, and there's unnecessary overhead using UDP if all you want is simple logging. Instead we provide an optional plugin for sending standard syslog over UDP to an external network interface.

* To enable the UDP syslog plugin, simply `set(PLUGINS .., syslogd,..)` in CMakeLists.txt or turn on the libsyslogd cmake option. This will override the default.

# Compatibility with Linux:
This example intends to show how the POSIX syslog interface works the same way in both Linux and IncludeOS. The CMake build creates an IncludeOS bootable image which inlcudes the program `syslog_example.c`. Service::start` in `service.cpp` calls `main` in `syslog_example.c`. The syslog example can also be built and run umodified under Linux:
* `$ make -f Makefile_linux`
Run locally by calling
* `$ ./syslog_linux`

NOTE: The example will send various types of log messages, including `LOG_ALERT`, `LOG_EMERG` etc. Also note that the IncludeOS service will transmit UDP packets to a remote IP specified by the user. The user is in charge of pointing this IP to a valid syslog server.

Build and run with IncludeOS:

```
mkdir build
cd build
cmake ..
make
../run.sh syslog_plugin_example
```
