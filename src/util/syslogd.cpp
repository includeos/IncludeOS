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

// ------------------------- Syslog_facility -----------------------------

bool Syslog_facility::ident_is_set() {
  if (ident_ not_eq nullptr)
    return true;
  
  return false;
}

// --------------------------- Syslog_user ------------------------------

void Syslog_user::syslog(int priority, const std::string& log_message) {
	printf("%s", log_message.c_str());
}

std::string Syslog_user::name() { return "User"; }

// --------------------------- Syslog_mail ------------------------------

void Syslog_mail::syslog(int priority, const std::string& log_message) {

	// Just for testing:
	printf("%s", log_message.c_str());
}

std::string Syslog_mail::name() { return "Mail"; }

// ----------------------------- Syslog ---------------------------------

std::unique_ptr<Syslog_facility> Syslog::last_open = std::make_unique<Syslog_user>();

void Syslog::syslog(int priority, const char* buf) {
	printf("SYSLOG WITH NO MORE ARGUMENTS\n");

  /*
  	All syslog-calls comes through here in the end, so
  	here we want to format the log-message with header and body
  */

  /*
  	%m:
	  The message body is generated from the message (argument) and following arguments
	  in the same manner as if these were arguments to printf(), except that the additional
	  conversion specification %m shall be recognized; it shall convert no arguments,
	  shall cause the output of the error message string associated with the value of
	  errno on entry to syslog(), and may be mixed with argument specifications of the
	  "%n$" form. If a complete conversion specification with the m conversion specifier
	  character is not just %m, the behavior is undefined. A trailing <newline> may be
	  added if needed.
  */

  if (not valid_priority(priority)) {
    printf("Invalid priority - returning. What to do if this occurs?\n");
    return;
  }

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

  message += ": ";

  // Check if f.ex. pid is set - add to msgbuf if set

  // What about priority? Format here or in the subclass's syslog-method?

  // Last: Add the message (buf)
  message += std::string{buf} + "\n";

  last_open->syslog(priority, message);
}