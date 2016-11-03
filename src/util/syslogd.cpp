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

#include <syslogd>
#include <service>

#include <ctime>

#include <errno.h>
#include <unistd.h>		// getpid

// ------------------------- Syslog_facility -----------------------------

bool Syslog_facility::ident_is_set() {
  if (ident_ not_eq nullptr)
    return true;
  
  return false;
}

std::string Syslog_facility::priority_string() {
	switch (priority_) {
		case LOG_EMERG:
			return "EMERG";
		case LOG_ALERT:
			return "ALERT";
		case LOG_CRIT:
			return "CRIT";
		case LOG_ERR:
			return "ERR";
		case LOG_WARNING:
			return "WARNING";
		case LOG_NOTICE:
			return "NOTICE";
		case LOG_INFO:
			return "INFO";
		case LOG_DEBUG:
			return "DEBUG";
		default:
			return "NONE";
	}
}

// --------------------------- Syslog_kern ------------------------------

void Syslog_kern::syslog(const std::string& log_message) {

	// Just for testing:
	printf("%s", log_message.c_str());
}

std::string Syslog_kern::name() { return "KERN"; }

// --------------------------- Syslog_user ------------------------------

void Syslog_user::syslog(const std::string& log_message) {
	printf("%s", log_message.c_str());
}

std::string Syslog_user::name() { return "USER"; }

// --------------------------- Syslog_mail ------------------------------

void Syslog_mail::syslog(const std::string& log_message) {

	// Just for testing:
	printf("%s", log_message.c_str());
}

std::string Syslog_mail::name() { return "MAIL"; }

// ----------------------------- Syslog ---------------------------------

std::unique_ptr<Syslog_facility> Syslog::last_open = std::make_unique<Syslog_user>();

void Syslog::syslog(int priority, const char* buf) {

	/*
  	All syslog-calls comes through here in the end, so
  	here we want to format the log-message with header and body
  */

	// Keeps % if calling this with %m and no arguments

  if (not valid_priority(priority)) {
  	// TODO (What to do if this occurs? Default?)
    printf("Invalid priority - returning\n");
    return;
  }

 	last_open->set_priority(priority);

  /* Building the message */

  // First: Timestamp
  // TODO: Correct time?
  char timebuf[TIMELEN];
  time_t now;
  time(&now);
  strftime(timebuf, TIMELEN, "%h %e %T ", localtime(&now));

  std::string message{timebuf};

  // Second: Ident
  if (last_open->ident_is_set())
    message += last_open->ident();
  else
  	message += Service::binary_name();

  // Third: PID
  if (last_open->logopt() & LOG_PID)
  	message += "[" + std::to_string(getpid()) + "]";

  message += ": ";

  // Fourth: Facility-name and priority/severity with colors
  message += pri_colors.at(last_open->priority()) +
  	"<" + last_open->name() + "." + last_open->priority_string() + "> " +
  	COLOR_END;

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
  // Fifth: Handle %m (replace it with strerror(errno)) and add the message (buf)
  std::regex m_regex{"\\%m"};
  message += std::regex_replace(buf, m_regex, strerror(errno)) + "\n";

  last_open->syslog(message);
}