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
#ifndef POSIX_SYSLOG_H
#define POSIX_SYSLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_PRIMASK		0x07		/* mask to extract priority part (internal) */

#define LOG_PRI(p)		((p) & LOG_PRIMASK)

#define LOG_FACMASK		0x03f8		/* facility mask: mask to extract facility part */

/*
	Macros for constructing the maskpri argument to setlogmask()
	The macros expand to an expression of type int when the argument
	pri is an expression of type int
*/
#define LOG_MASK(pri)	(1 << (pri))			/* mask for one priority */
//	Added: Mask of all priorities up to and including pri:
#define LOG_UPTO(pri)	((1 << ((pri)+1)) - 1)	/* all priorities through pri */

/*
	Possible facility values
	Symbolic constants for use as the facility argument to openlog()
	As specified in RFC5424
*/
#define LOG_KERN			0		/* Kernel messages */
#define LOG_USER			1 	/* User-level messages */
#define LOG_MAIL			2 	/* Mail system */
#define LOG_DAEMON		3 	/* System daemons */
#define LOG_AUTH			4 	/* Security/authorization messages */
#define LOG_LPR				6 	/* Line printer subsystem */
#define LOG_NEWS			7 	/* Network news subsystem */
#define LOG_UUCP			8 	/* UUCP subsystem */
#define LOG_CRON			9 	/* Clock daemon */

#define LOG_LOCAL0		16	/* Local use 0 */
#define LOG_LOCAL1 		17	/* Local use 1 */
#define LOG_LOCAL2 		18	/* Local use 2 */
#define LOG_LOCAL3 		19	/* Local use 3 */
#define LOG_LOCAL4  	20	/* Local use 4 */
#define LOG_LOCAL5 		21	/* Local use 5 */
#define LOG_LOCAL6 		22	/* Local use 6 */
#define LOG_LOCAL7 		23	/* Local use 7 */

/*
	Symbolic constants, where zero or more of which may be OR'ed together
	to form the logopt option of openlog()
*/
#define LOG_PID				0X01	/* Log the process ID with each message */
#define LOG_CONS			0X02 	/* Log to the system console on error */
#define LOG_NDELAY		0X04 	/* Connect to syslog daemon immediately */
#define LOG_ODELAY		0X08 	/* Delay open until syslog() is called
														 	 Is the converse of LOG_NDELAY and does no longer do anything */
#define LOG_NOWAIT		0X10	/* Do not wait for child processes
														 	 Deprecated */
#define LOG_PERROR		0X20	/* Is not specified by POSIX.1-2001 or POSIX.1-2008, but is
															 available in most versions of UNIX
															 Log to stderr */

/*
	Possible values of severity
	Symbolic constants for use as the priority argument of syslog()
	As specified in RFC5424 and POSIX
*/
#define LOG_EMERG			0		/* Emergency: System is unusable */
#define LOG_ALERT			1 	/* Alert: Action must be taken immediately */
#define LOG_CRIT			2 	/* Critical: Critical conditions */
#define LOG_ERR				3 	/* Error: Error conditions */
#define LOG_WARNING		4 	/* Warning: Warning conditions */
#define LOG_NOTICE		5 	/* Notice: Normal but significant condition */
#define LOG_INFO			6 	/* Informational: Informational messages */
#define LOG_DEBUG			7 	/* Debug: Debug-level messages */

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
