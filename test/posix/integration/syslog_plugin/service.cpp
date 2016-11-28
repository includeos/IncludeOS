// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <service>

/* For testing Posix */
#include <syslog.h>

/* For testing IncludeOS */
#include <syslogd>

#include <net/inet4>

int main()
{
  /* --- Integration test for the plugin syslog behavior: Sending data over UDP --- */

  /* ------------------------- Testing POSIX syslog ------------------------- */

  // DHCP on interface 0
  auto& inet = net::Inet4::stack();
  // static IP in case DHCP fails
  inet.network_config({  10,  0,  0, 46 },   // IP
                                   { 255, 255, 255,  0 },    // Netmask
                                   {  10,  0,  0,  1 },    // Gateway
                                   {  10,  0,  0,  1} );   // DNS

  // Setting IP and port for the syslog messages
  Syslog::settings( {10, 0, 0, 1}, 6514 );
  printf("Syslog messages are sent to IP %s and port %d\n", Syslog::ip().str().c_str(), Syslog::port());

  // Starts the python integration test:
  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());

  int invalid_priority = -1;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 10;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 55;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  std::string one{"one"};
  std::string two{"two"};
  size_t number = 33;

  syslog(LOG_INFO, "(Info) No open has been called prior to this");
  syslog(LOG_NOTICE, "(Notice) Program created with two arguments: %s and %s", one.c_str(), two.c_str());

  openlog("Prepended message", LOG_CONS | LOG_NDELAY, LOG_MAIL);

  syslog(LOG_ERR, "(Err) Log after prepended message with one argument: %d", 44);
  syslog(LOG_WARNING, "(Warning) Log number two after openlog set prepended message");

  closelog();

  syslog(LOG_WARNING, "(Warning) Log after closelog with three arguments. One is %u, another is %s, a third is %d", number, "this", 4011);

  openlog("Second prepended message", LOG_PID | LOG_CONS, LOG_USER);

  syslog(LOG_EMERG, "Emergency log after openlog and new facility: user");
  syslog(LOG_ALERT, "Alert log with the m argument: %m");

  closelog();

  syslog(LOG_CRIT, "Critical after cleared prepended message (closelog has been called)");

  closelog();

  openlog("Open after close prepended message", LOG_CONS, LOG_MAIL);

  syslog(LOG_INFO, "Info after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);

  closelog();

  /* ------------------------- Testing IncludeOS syslog ------------------------- */

  invalid_priority = -1;
  Syslog::syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 10;
  Syslog::syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 55;
  Syslog::syslog(invalid_priority, "Invalid %d", invalid_priority);

  Syslog::syslog(LOG_INFO, "(Info) No open has been called prior to this");
  Syslog::syslog(LOG_NOTICE, "(Notice) Program created with two arguments: %s and %s", one.c_str(), two.c_str());

  Syslog::openlog("Prepended message", LOG_CONS | LOG_NDELAY, LOG_MAIL);

  Syslog::syslog(LOG_ERR, "(Err) Log after prepended message with one argument: %d", 44);
  Syslog::syslog(LOG_WARNING, "(Warning) Log number two after openlog set prepended message");

  Syslog::closelog();

  Syslog::syslog(LOG_WARNING, "(Warning) Log after closelog with three arguments. One is %u, another is %s, a third is %d", number, "this", 4011);

  Syslog::openlog("Second prepended message", LOG_PID | LOG_CONS, LOG_USER);

  Syslog::syslog(LOG_EMERG, "Emergency log after openlog and new facility: user");
  Syslog::syslog(LOG_ALERT, "Alert log with the m argument: %m");

  Syslog::closelog();

  Syslog::syslog(LOG_CRIT, "Critical after cleared prepended message (closelog has been called)");

  Syslog::closelog();

  Syslog::openlog("Open after close prepended message", LOG_CONS, LOG_MAIL);

  Syslog::syslog(LOG_INFO, "Info after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);

  /* Signal test finished: */
  Syslog::syslog(LOG_DEBUG, "(Debug) Something special to close with");

  return 0;
}

void Service::start(const std::string&)
{
  main();
}
