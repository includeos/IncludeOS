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
#include <sys/syslog.h>

/* For testing IncludeOS */
#include <syslogd>

int main()
{
  /* ------------------------- Testing POSIX syslog ------------------------- */
    
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

  openlog("Open after close prepended message", 0, LOG_USER);

  syslog(LOG_INFO, "Info after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);

  closelog();

  /* ------------------------- Testing IncludeOS syslog ------------------------- */

  Syslog::syslog(LOG_INFO, "No <Syslogd> open has been called prior to this");
  Syslog::syslog(LOG_NOTICE, "Program <Syslogd> created with two arguments: %s and %s", one.c_str(), two.c_str());
  
  Syslog::openlog<Syslog_mail>("Prepended message", LOG_CONS);

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

  Syslog::openlog<Syslog_user>("Open after close prepended message", 0);

  Syslog::syslog(LOG_INFO, "Info <Syslogd> after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);

  /* Signal test finished: */
  Syslog::syslog(LOG_DEBUG, "Something special to close with");

  return 0;
}

void Service::start(const std::string&)
{
  main();
}
