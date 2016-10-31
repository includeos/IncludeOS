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

#pragma once
#ifndef POSIX_SYS_SYSLOG_H
#define POSIX_SYS_SYSLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/syslog_consts.h>

/*
	Shall close any open file descriptors allocated by previous calls to
	openlog() or syslog()
*/
void closelog(void);

/*
	Shall set process attributes that affect subsequent calls to syslog().
	The ident argument is a string that is prepended to every message.
	The logopt argument indicates logging options.
	Values for logopt are constructed by a bitwise-inclusive OR of zero or
	more of the following: look above.

	The facility argument encodes a default facility to be assigned to all
	messages that do not have an explicit facility already encoded. The initial
	default facility is LOG_USER.
*/
void openlog(const char* ident, int logopt, int facility);

/*
	Shall set the log priority mask for the current process to maskpri and
	return the previous mask.
	If the maskpri argument is 0, the current log mask is not modified.
	Calls by the current process to syslog() with a priority not set in maskpri
	shall be rejected.
	The default log mask allows all priorities to be logged.
	A call to openlog() is not required prior to calling setlogmask().

	The function shall return the previous log priority mask.
*/
int setlogmask(int maskpri);

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
void syslog(int priority, const char* message, ... /* arguments*/);

#ifdef __cplusplus
}
#endif

#endif