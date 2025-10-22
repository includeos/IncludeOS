# Syslog Integration Test Printf

Testing POSIX syslog and IncludeOS syslogd implementation.

This tests the default behavior a.k.a. sending syslog data to printf.

The message format is custom (not corresponding with that specified in RFC5424) because we are not
sending data over UDP and therefore some of the data that RFC5424 specifies is not relevant. However, with this
logging to printf you get each log message's facility and severity level color coded.

This is the default behavior for our syslog implementation and is what you get if you include syslogd (the IncludeOS implementation of syslog) or syslog.h (the POSIX wrapper around the IncludeOS syslog implementation).
If you want the data to be sent over UDP, add `set(PLUGINS syslogd)` to your CMakeLists.txt file (see the syslog_plugin integration test). Then you also get the message format specified in RFC5424.
