# Syslog IncludeOS Plugin Service

Sending syslog data over UDP port 6514.

The message format corresponds to the format specified in RFC5424.

Main in service.cpp corresponds to main in syslog_lubuntu/syslog_lubuntu.c.

PLUGIN=syslogd must be defined in Makefile.

(The default behavior for IncludeOS's syslog implementation is however to send the data to printf, which is
what you get if you don't specify PLUGIN=syslogd in your Makefile (see the syslog_default integration test)).
