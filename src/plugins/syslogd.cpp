// Syslog plugin (UDP)
#include <os>
#include <syslogd>

void register_plugin_syslogd() {
  INFO("Syslog", "Sending buffered data to syslog plugin");

  Syslog::set_facility(std::make_unique<Syslog_udp>());

  /*
    @todo
    Get dmesg (kernel logs) and send to syslog
    INFO needs to be rewritten to use kprint and kprint needs to be rewritten to buffer the data
  */

}

__attribute__((constructor))
void register_syslogd(){
  os::register_plugin(register_plugin_syslogd, "Syslog over UDP");
}
