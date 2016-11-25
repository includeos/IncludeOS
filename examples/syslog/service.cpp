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
#include <net/inet4>

/* For Posix */
#include <syslog.h>

/* For IncludeOS */
// #include <syslogd>

int main()
{
  // DHCP on interface 0
  auto& inet = net::Inet4::ifconfig(10.0);
  // static IP in case DHCP fails
  net::Inet4::ifconfig({  10,  0,  0, 45 },   // IP
                      { 255, 255, 0,  0 },    // Netmask
                      {  10,  0,  0,  1 },    // Gateway
                      {  10,  0,  0,  1} );   // DNS

  // Starts the python integration test:
  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());

  /* ------------------------- POSIX syslog in IncludeOS ------------------------- */

  int invalid_priority = -1;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 10;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 55;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  syslog(LOG_INFO, "(Info) No open has been called prior to this");
  syslog(LOG_NOTICE, "(Notice) Program created with two arguments: %s and %s", "one", "two");

  openlog("Prepended message", LOG_CONS | LOG_NDELAY, LOG_MAIL);

  syslog(LOG_ERR, "(Err) Log after prepended message with one argument: %d", 44);
  syslog(LOG_WARNING, "(Warning) Log number two after openlog set prepended message");

  closelog();

  syslog(LOG_WARNING, "(Warning) Log after closelog with three arguments. One is %u, another is %s, a third is %d", 33, "this", 4011);

  openlog("Second prepended message", LOG_PID | LOG_CONS, LOG_USER);

  syslog(LOG_EMERG, "Emergency log after openlog and new facility: user");
  syslog(LOG_ALERT, "Alert log with the m argument: %m");

  closelog();

  syslog(LOG_CRIT, "Critical after cleared prepended message (closelog has been called)");

  closelog();

  openlog("Open after close prepended message", LOG_CONS, LOG_MAIL);

  syslog(LOG_INFO, "Info after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);

  closelog();

  /* ------------------------- Alternatively: IncludeOS syslog without POSIX wrapper -------------------------

  invalid_priority = -1;
  Syslog::syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 10;
  Syslog::syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 55;
  Syslog::syslog(invalid_priority, "Invalid %d", invalid_priority);

  Syslog::syslog(LOG_INFO, "(Info) No open has been called prior to this");
  Syslog::syslog(LOG_NOTICE, "(Notice) Program created with two arguments: %s and %s", "one", "two");

  Syslog::openlog("Prepended message", LOG_CONS | LOG_NDELAY, LOG_MAIL);

  Syslog::syslog(LOG_ERR, "(Err) Log after prepended message with one argument: %d", 44);
  Syslog::syslog(LOG_WARNING, "(Warning) Log number two after openlog set prepended message");

  Syslog::closelog();

  Syslog::syslog(LOG_WARNING, "(Warning) Log after closelog with three arguments. One is %u, another is %s, a third is %d", 33, "this", 4011);

  Syslog::openlog("Second prepended message", LOG_PID | LOG_CONS, LOG_USER);

  Syslog::syslog(LOG_EMERG, "Emergency log after openlog and new facility: user");
  Syslog::syslog(LOG_ALERT, "Alert log with the m argument: %m");

  Syslog::closelog();

  Syslog::syslog(LOG_CRIT, "Critical after cleared prepended message (closelog has been called)");

  Syslog::closelog();

  Syslog::openlog("Open after close prepended message", LOG_CONS, LOG_MAIL);

  Syslog::syslog(LOG_INFO, "Info after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);

  */

  return 0;
}

void Service::start(const std::string&)
{
  main();
}
