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

int main()
{
  /* ------------------------- Testing POSIX syslog ------------------------- */
  
  auto& inet = Inet4::stack<0>();
  inet.network_config({  10,  0,  0, 45 },   // IP
                      { 255, 255, 0,  0 },   // Netmask
                      {  10,  0,  0,  1 } ); // Gateway

  // Starts the python integration test:
  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());

  // Testing all priority-calculations:
/*
  openlog("USER", LOG_NDELAY, LOG_USER);
  syslog(0, "Pri 0, Fac USER (1). Result should be: (1*8) + 0 = 8");
  syslog(1, "Pri 1, Fac USER (1). Result should be: (1*8) + 1 = 9");
  syslog(2, "Pri 2, Fac USER (1). Result should be: (1*8) + 2 = 10");
  syslog(3, "Pri 3, Fac USER (1). Result should be: (1*8) + 3 = 11");
  syslog(4, "Pri 4, Fac USER (1). Result should be: (1*8) + 4 = 12");
  syslog(5, "Pri 5, Fac USER (1). Result should be: (1*8) + 5 = 13");
  syslog(6, "Pri 6, Fac USER (1). Result should be: (1*8) + 6 = 14");
  syslog(7, "Pri 7, Fac USER (1). Result should be: (1*8) + 7 = 15");

  openlog("MAIL", LOG_NDELAY, LOG_MAIL);
  syslog(0, "Pri 0, Fac MAIL (2). Result should be: (2*8) + 0 = 16");
  syslog(1, "Pri 1, Fac MAIL (2). Result should be: (2*8) + 1 = 17");
  syslog(2, "Pri 2, Fac MAIL (2). Result should be: (2*8) + 2 = 18");
  syslog(3, "Pri 3, Fac MAIL (2). Result should be: (2*8) + 3 = 19");
  syslog(4, "Pri 4, Fac MAIL (2). Result should be: (2*8) + 4 = 20");
  syslog(5, "Pri 5, Fac MAIL (2). Result should be: (2*8) + 5 = 21");
  syslog(6, "Pri 6, Fac MAIL (2). Result should be: (2*8) + 6 = 22");
  syslog(7, "Pri 7, Fac MAIL (2). Result should be: (2*8) + 7 = 23");
*/

/*
  Notes:
  * LOG_CONS writes to console so affects python integration test (test.py)
  * 
*/

  int invalid_priority = -1;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 10;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 55;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  std::string one{"one"};
  std::string two{"two"};
  size_t number = 33;

  syslog(LOG_INFO, "No open has been called prior to this");
  syslog(LOG_NOTICE, "Program created with two arguments: %s and %s", one.c_str(), two.c_str());
  
  openlog("Prepended message", LOG_CONS, LOG_MAIL);

  syslog(LOG_ERR, "Log after prepended message with one argument: %d", 44);
  syslog(LOG_WARNING, "Log number two after openlog set prepended message");

  closelog();

  syslog(LOG_WARNING, "Log after closelog with three arguments. One is %u, another is %s, a third is %d", number, "this", 4011);

  openlog("Second prepended message", LOG_PID | LOG_CONS, LOG_USER);

  syslog(LOG_EMERG, "Emergency log after openlog and new facility: user");
  syslog(LOG_ALERT, "Alert log with the m argument: %m");

  closelog();

  syslog(LOG_CRIT, "Critical after cleared prepended message (closelog has been called)");

  closelog();

  openlog("Open after close prepended message", LOG_CONS, LOG_USER);

  syslog(LOG_INFO, "Info after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);

  closelog();

  /* ------------------------- Testing IncludeOS syslog ------------------------- */
/*
  Syslog::syslog(LOG_INFO, "No <Syslogd> open has been called prior to this");
  Syslog::syslog(LOG_NOTICE, "Program <Syslogd> created with two arguments: %s and %s", one.c_str(), two.c_str());
  
  Syslog::openlog<Syslog_mail>("Prepended message", LOG_CONS | LOG_NDELAY);

  Syslog::syslog(LOG_ERR, "Log <Syslogd> after prepended message with one argument: %d", 44);
  Syslog::syslog(LOG_WARNING, "Log <Syslogd> number two after openlog set prepended message");

  Syslog::closelog();

  Syslog::syslog(LOG_WARNING, "Log <Syslogd> after closelog with three arguments. One is %u, another is %s, a third is %d", number, "this", 4011);

  Syslog::openlog<Syslog_user>("Second prepended message", LOG_PID | LOG_CONS);

  Syslog::syslog(LOG_EMERG, "Emergency <Syslogd> log after openlog and new facility: user");
  Syslog::syslog(LOG_ALERT, "Alert <Syslogd> log with the m argument: %m");

  Syslog::closelog();

  Syslog::syslog(LOG_CRIT, "Critical <Syslogd> after cleared prepended message (closelog has been called)");

  Syslog::closelog();

  Syslog::openlog<Syslog_user>("Open after close prepended message", LOG_CONS);

  Syslog::syslog(LOG_INFO, "Info <Syslogd> after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);
*/
  /* Signal test finished: */
  Syslog::syslog(LOG_DEBUG, "Something special to close with");
  
  return 0;
}

void Service::start(const std::string&)
{
  main();
}
