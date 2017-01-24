// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

// Weak default printf

#include <syslogd>
#include <service>
#include <errno.h>		// errno
#include <unistd.h>		// getpid

#include <ctime>

std::unique_ptr<Syslog_facility> Syslog::fac_ = std::make_unique<Syslog_facility>();

// va_list arguments (POSIX)
void Syslog::syslog(const int priority, const char* message, va_list args) {
  // vsnprintf removes % if calling syslog with %m in addition to arguments
  // Find %m here first and escape % if found
  std::regex m_regex{"\\%m"};
  std::string msg = std::regex_replace(message, m_regex, "%%m");

  char buf[BUFLEN];
  vsnprintf(buf, BUFLEN, msg.c_str(), args);
  syslog(priority, buf);
}

void Syslog::syslog(const int priority, const char* buf) {

	/*
		Custom syslog-message without some of the data specified in RFC5424
		becase this implementation doesn't send data over UDP
	*/

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

 	/* Building the log message */

 	/* PRI FAC_NAME PRI_NAME TIMESTAMP APP-NAME IDENT PROCID */

 	// First: Priority- and facility-value (PRIVAL)
 	std::string message = "<" + std::to_string(fac_->calculate_pri()) + "> ";

 	// Second : Facility and priority/severity in plain text with colors
 	message += pri_colors.at(fac_->priority()) + "<" + fac_->facility_name() + "." +
 		fac_->priority_name() + "> " + COLOR_END;

 	// Third: Timestamp
 	char timebuf[TIMELEN];
 	time_t now;
 	time(&now);
 	strftime(timebuf, TIMELEN, "%FT%T.000Z", localtime(&now));
 	message += std::string{timebuf} + " ";

 	// Fourth: App-name
 	message += Service::binary_name();

 	// Fifth: Add ident if is set (through openlog)
 	if (fac_->ident_is_set())
 		message += " " + std::string{fac_->ident()};

 	// Sixth: Add PID (PROCID) if LOG_PID is specified (through openlog)
 	if (fac_->logopt() & LOG_PID)
 		message += "[" + std::to_string(getpid()) + "]";

 	message += ": ";

 	/* Message */

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
  // Handle %m (replace it with strerror(errno)) and add the message (buf)
  std::regex m_regex{"\\%m"};
  message += std::regex_replace(buf, m_regex, strerror(errno));

 	/* Last: Send the log string */
 	fac_->syslog(message);
}

void Syslog::openlog(const char* ident, int logopt, int facility) {
  // fac_ = std::make_unique<Syslog_facility>(ident, facility);
  fac_->set_ident(ident);

  if (valid_logopt(logopt) or logopt == 0)  // Should be possible to clear the logopt
    fac_->set_logopt(logopt);

  if (valid_facility(facility))
    fac_->set_facility(facility);

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
}
