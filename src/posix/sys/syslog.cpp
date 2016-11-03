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

#include <sys/syslog.h>
#include <syslogd>

// const int INTERNALLOG = LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID;

static const char* ident_ = nullptr;	/* what is prepended to each message
																				default could be the name of the program */
																			
static int logopt_ = 0;								/* default logging option */
static int facility_ = LOG_USER;			/* default facility */

static int log_mask = 0xff;						/* mask of priorities to be logged
																				 default: allows all priorities to be logged */

void closelog(void) {
	// Back to default values:
	ident_ = nullptr;
	logopt_ = 0;
	facility_ = LOG_USER;
	Syslog::closelog();
}

/*
	The logopt argument indicates logging options. Values for logopt are constructed
	by a bitwise-inclusive OR of zero or more of the following: look above.

	Options (logopt) is a bit string, with the bits as defined by the single bit masks
		LOG_PERROR	// ?
		LOG_CONS
		LOG_PID
		LOG_NDELAY
		LOG_ODELAY
		LOG_NOWAIT	// ?
	If any other bit in options is on, the result is undefined
*/
/*
	The following example causes subsequent calls to syslog() to log the process ID with each message,
	and to write messages to the system console if they cannot be sent to the logging facility.

	#include <syslog.h>
	char *ident = "Process demo";
	int logopt = LOG_PID | LOG_CONS;
	int facility = LOG_USER;
	...
	openlog(ident, logopt, facility);
*/
void openlog(const char* ident, int logopt, int facility) {
	if (ident != nullptr)
		ident_ = ident;

	logopt_ = logopt;

	// Check if valid facility and set if is
	if (facility != 0 and (facility &~ LOG_FACMASK) == 0)
		facility_ = facility;

	switch (facility_) {
		case LOG_KERN:
			Syslog::openlog<Syslog_kern>(ident_, logopt_);
			break;
		case LOG_MAIL:
			Syslog::openlog<Syslog_mail>(ident_, logopt_);
			break;
		// More facilities
		default:
			// If facility_ doesn't match any of the above, go to default:
			Syslog::openlog<Syslog_user>(ident_, logopt_);
			break;
	}
}

/*
	Shall set the log priority mask for the current process to maskpri and
	return the previous mask.
	If the maskpri argument is 0, the current log mask is not modified.
	Calls by the current process to syslog() with a priority not set in maskpri
	shall be rejected.
	
	The default log mask allows all priorities to be logged.
	(on top set to 0xff)
	
	A call to openlog() is not required prior to calling setlogmask().

	The function shall return the previous log priority mask.

	Using setlogmask():
	The following example causes subsequent calls to syslog() to accept error messages, and to reject all other messages.
	#include <syslog.h>
	int result;
	int mask = LOG_MASK (LOG_ERR);
	...
	result = setlogmask(mask);
*/
int setlogmask(int maskpri) {
	int old_mask = log_mask;

	if (maskpri != 0)		// the mask has been modified
		log_mask = maskpri;

	return old_mask;
}

/*
	Shall send a message to an implementation-defined logging facility,
	which may log it in an implementation-defined system log, write it to the
	system console, forward it to a list of users, or forward it to the logging
	facility on another host over the network. The logged message shall include
	a message header and a message body. The message header contains at least a
	timestamp and a tag string.

	The message body is generated from the message (argument) and following arguments
	in the same manner as if these were arguments to printf(), except that the additional
	conversion specification %m shall be recognized; it shall convert no arguments,
	shall cause the output of the error message string associated with the value of
	errno on entry to syslog(), and may be mixed with argument specifications of the
	"%n$" form. If a complete conversion specification with the m conversion specifier
	character is not just %m, the behavior is undefined. A trailing <newline> may be
	added if needed.

	Values of the priority argument are formed by OR'ing together a severity-level value
	and an optional facility value. If no facility value is specified, the current
	default facility value is used.
	Possible values of severity: look above.
	Possible facility values: look above.
*/
/*
	Calls by the current process to syslog() with a priority not set in maskpri shall be rejected.
*/
/*
	You don't have to use openlog. If you call syslog without having called openlog, syslog
	just opens the connection implicitly and uses defaults for the information in ident and
	options
*/
/*
	The following example sends the message "This is a message" to the default logging facility, marking the message as an error message generated by random processes.

	#include <syslog.h>
	char *message = "This is a message";
	int priority = LOG_ERR | LOG_USER;
	...
	syslog(priority, message);
*/
void syslog(int priority, const char* message, ... /* arguments*/) {

	// Check of priority happens in Syslog (syslogd.hpp) (valid_priority(int priority))

	/*
		Values of the priority argument are formed by OR'ing together a
		severity-level value and an optional facility value. If no facility
		value is specified, the current default facility value is used.
	*/

	/* Arguments */
	va_list ap;
	va_start(ap, message);
	va_end(ap);

	// Calling syslog-function that takes a va_list (uses vsnprintf)
	Syslog::syslog(priority, message, ap);
}