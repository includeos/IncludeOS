// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 IncludeOS AS, Oslo, Norway
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

#include <syslogd>
#include <service>
#include <errno.h>		// errno
#include <unistd.h>		// getpid

std::unique_ptr<Syslog_facility> Syslog::fac_ = std::make_unique<Syslog_print>();

void Syslog::syslog(const int priority, const char* fmt, ...)
{
  va_list list;
  va_start(list, fmt);
  syslog(priority, fmt, list);
  va_end(list);
}

// va_list arguments (POSIX)
void Syslog::syslog(const int priority, const char* fmt, va_list args)
{
  // due to musl "bug" (strftime setting errno..)
  const int save_errno = errno;
  // snprintf removes % if calling syslog with %m in addition to arguments
  // Find %m here first and escape % if found
  std::regex m_regex{"\\%m"};
  std::string msg = std::regex_replace(fmt, m_regex, "%%m");

  char buf[2048];
  vsnprintf(buf, sizeof(buf), msg.c_str(), args);

	/*
  	All syslog-calls comes through here in the end, so
  	here we want to format the log-message with header and body
  */

	// % is kept if calling this with %m and no arguments

	/* If the priority is not valid, call syslog again with LOG_ERR as priority
		 and an unknown priority-message
		 Could also document by setting logopt to LOG_PID | LOG_CONS | LOG_PERROR, but
		 then only for this specific message */
  if (not valid_priority(priority)) {
  	syslog(LOG_ERR, "Syslog: Unknown priority %d. Message: %s", priority, buf);
    return;
  }
 	fac_->set_priority(priority);

  /* Building the log message based on the facility used */
  std::string message = fac_->build_message_prefix(Service::binary_name());

 	/*
 		%m:
		(The message body is generated from the message (argument) and following arguments
		in the same manner as if these were arguments to printf(), except that the additional
		conversion specification %m shall be recognized;)
		it shall convert no arguments, shall cause the output of the error message string
		associated with the value of errno on entry to syslog(), and may be mixed with argument
		specifications of the "%n$" form. If a complete conversion specification with the m conversion
		specifier character is not just %m, the behavior is undefined. A trailing <newline> may be
		added if needed.
	*/
  errno = save_errno;
  // Handle %m (replace it with strerror(errno)) and add the message (buf)
  message += std::regex_replace(buf, m_regex, strerror(errno));

 	/* Last: Send the log string */
 	fac_->syslog(message);
}

void Syslog::openlog(const char* ident, int logopt, int facility) {
  fac_->set_ident(ident);

  if (valid_logopt(logopt) or logopt == 0)  // Should be possible to clear the logopt
    fac_->set_logopt(logopt);
  else
    fac_->set_logopt(0);  // Because of call to openlog from closelog

  if (valid_facility(facility))
    fac_->set_facility(facility);

  if (logopt & LOG_NDELAY) {
    // Connect to syslog daemon immediately
    fac_->open_socket();
  }
  /*else if (logopt & LOG_ODELAY) {
    // Delay open until syslog() is called = do nothing (the converse of LOG_NDELAY)
  }*/

  /* Check for this in send_udp_data() in Syslog_facility instead
  if (logopt & LOG_CONS and fac_->priority() == LOG_ERR) {
    // Log to system console
  }*/

  /* Not relevant if logging to printf
  if (logopt & LOG_NDELAY) {
    // Connect to syslog daemon immediately
  } else if (logopt & LOG_ODELAY) {
    // Delay open until syslog() is called = do nothing (the converse of LOG_NDELAY)
  }*/

  /*if (logopt & LOG_NOWAIT) {
    // Do not wait for child processes - deprecated
  }*/
}

void Syslog::closelog() {
  // Back to default values:
  // ident_ = nullptr;
  // logopt = 0;
  // facility = LOG_USER;
  openlog(nullptr, 0, LOG_USER);
  fac_->close_socket();
}
