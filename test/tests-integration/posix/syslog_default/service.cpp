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
#include <fs/vfs.hpp>
int main()
{
	/* ----------- Integration test for the default syslog behavior: printf ----------- */

	/* ------------------------- Testing POSIX syslog ------------------------- */
  INFO("Syslog", "POSIX");
  int invalid_priority = -1;

  syslog(invalid_priority, "Invalid %d", invalid_priority);

  syslog(LOG_INFO, " : A info message");

  std::string one{"one"};
  std::string two{"two"};
  syslog(LOG_NOTICE, "Program created with two arguments: %s and %s", one.c_str(), two.c_str());

  INFO2("Adding prepend");

  openlog("Prepended message", 0, LOG_MAIL);

  syslog(LOG_ERR, "Log after prepended message with one argument: %d", 44);

  closelog();

  size_t number = 33;
  syslog(LOG_WARNING, "Log after closelog with three arguments. One is %u, another is %s, a third is %d", number, "this", 4011);

  openlog("Second prepended message", LOG_PID | LOG_NDELAY, LOG_USER);

  syslog(LOG_EMERG, "Emergency log after openlog and new facility: user");
  errno = EINVAL;
  syslog(LOG_ALERT, "Alert log with the m argument: %m");
  errno = 0;
  syslog(LOG_ALERT, "Second alert log with the m argument: %m");


  closelog();

  syslog(LOG_CRIT, "Critical after cleared prepended message (closelog has been called)");

  closelog();

  // max 32 chars ident allowed
  openlog("Open after close prepended message", LOG_PERROR, LOG_KERN);

  syslog(LOG_INFO, "Info after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);

  closelog();

  /* ------------------------- Testing IncludeOS syslog ------------------------- */
  INFO("Syslog", "Syslogd");
  invalid_priority = -1;
  Syslog::syslog(invalid_priority, "Syslogd Invalid %d", invalid_priority);

  invalid_priority = 10;
  Syslog::syslog(invalid_priority, "Syslogd Invalid %d", invalid_priority);

  invalid_priority = 55;
  Syslog::syslog(invalid_priority, "Syslogd Invalid %d", invalid_priority);

  Syslog::syslog(LOG_INFO, "Syslogd No open has been called prior to this");
  Syslog::syslog(LOG_NOTICE, "Syslogd Program created with two arguments: %s and %s", one.c_str(), two.c_str());

  Syslog::openlog("Prepended message", 0, LOG_MAIL);

  Syslog::syslog(LOG_ERR, "Syslogd Log after prepended message with one argument: %d", 44);
  Syslog::syslog(LOG_WARNING, "Syslogd Log number two after openlog set prepended message");

  Syslog::closelog();

  Syslog::syslog(LOG_WARNING, "Syslogd Log after closelog with three arguments. One is %zu, another is %s, a third is %d", number, "this", 4011);

  Syslog::openlog("Second prepended message", LOG_PID | LOG_NDELAY, LOG_USER);

  Syslog::syslog(LOG_EMERG, "Syslogd Emergency log after openlog and new facility: user");
  Syslog::syslog(LOG_ALERT, "Syslogd Alert log with the m argument: %m");

  Syslog::closelog();

  Syslog::syslog(LOG_CRIT, "Syslogd Critical after cleared prepended message (closelog has been called)");

  Syslog::closelog();

  Syslog::openlog("Open after close prepended message", LOG_PERROR, LOG_KERN);

  errno = 0;
  Syslog::syslog(LOG_INFO, "Syslogd Info after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);
  printf("%d\n", errno);

  Syslog::openlog("Exiting test", 0, LOG_LOCAL7);

  /* Signal test finished: */
  Syslog::syslog(LOG_DEBUG, "Something special to close with");

  return 0;
}

void Service::start(const std::string&)
{
  main();
}
