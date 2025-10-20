# Syslog Integration Test UDP

Testing POSIX syslog and IncludeOS syslogd implementation.

This tests the plugin behavior a.k.a. sending syslog data over UDP port 6514.

The message format corresponds to the format specified in RFC5424.

To achieve this behavior you have to add `set(PLUGINS syslogd)` to your CMakeLists.txt file.

The default behavior for IncludeOS's syslog implementation is however to send the data to printf, which is
what you get if you don't add `set(PLUGINS syslogd)` to your CMakeLists.txt file (see the syslog_default integration test).
