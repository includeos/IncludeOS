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

// Syslog plugin (UDP)

#include <os>
#include <plugins/syslogd.hpp>
#include <service>
#include <net/inet4>
#include <errno.h>		// errno
#include <unistd.h>		// getpid

#include <syslog.h> 								// POSIX symbolic constants
#include <util/syslogd.hpp>					// Header with weak method declarations
#include <util/syslog_facility.hpp>	// Header with weak method declarations
#include <info>                     // INFO

// ------------------------- Syslog_facility -----------------------------

void Syslog_facility::syslog(const std::string& log_message) {
	if (logopt_ & LOG_CONS /*and priority_ == LOG_ERR*/)
		std::cout << log_message.c_str() << '\n';

  if (logopt_ & LOG_PERROR)
    std::cerr << log_message.c_str() << '\n';

  send_udp_data(log_message);
}

Syslog_facility::~Syslog_facility() {
  if (sock_)
    sock_->udp().close(sock_->local_port());
}

void Syslog_facility::open_socket() {
  if (sock_ == nullptr)
    sock_ = &net::Inet4::stack<>().udp().bind();
}

void Syslog_facility::close_socket() {
	if (sock_) {
    sock_->udp().close(sock_->local_port());
    sock_ = nullptr;
	}
}

void Syslog_facility::send_udp_data(const std::string& data) {
  open_socket();
  sock_->sendto( ip_, port_, data.c_str(), data.size() );
}

// ----------------------------- Syslog ---------------------------------

void Syslog::syslog(const int priority, const char* buf) {

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

 	/* Building the log message based on RFC5424 */

 	/* Header: PRI VERSION SP TIMESTAMP SP HOSTNAME SP APP-NAME SP PROCID SP MSGID */

 	// First: Priority- and facility-value (PRIVAL) and Syslog-version
 	std::string message = "<" + std::to_string(fac_->calculate_pri()) + ">1 ";

 	// Second: Timestamp
 	char timebuf[TIMELEN];
 	time_t now;
 	time(&now);
 	strftime(timebuf, TIMELEN, "%FT%T.000Z", localtime(&now));

 	// Third: Hostname ( Preferably: 1. FQDN (RFC1034) 2. Static IP address 3. Hostname 4. Dynamic IP address 5. NILVALUE (-) )
 	message += std::string{timebuf} + " " + net::Inet4::stack().ip_addr().str() + " ";

 	// Fourth: App-name, PROCID (LOG_PID) and MSGID
 	message += Service::binary_name() + " " + std::to_string(getpid()) + " UDPOUT ";

 	/* Structured data: SD-element (SD-ID PARAM-NAME=PARAM-VALUE) */
 	message += "- ";	// NILVALUE

 	/* Message */

 	// Add ident before message (buf) if is set (through openlog)
 	if (fac_->ident_is_set())
 		message += std::string{fac_->ident()} + " ";

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
	fac_->set_ident(ident);

  if (valid_logopt(logopt) or logopt == 0)  // Should be possible to clear the logopt
    fac_->set_logopt(logopt);
  else
  	fac_->set_logopt(0);	// Because of call to openlog from closelog

  if (valid_facility(facility))
    fac_->set_facility(facility);

  if (logopt & LOG_NDELAY) {
    // Connect to syslog daemon immediately
    fac_->open_socket();
  }
  /*else if (logopt & LOG_ODELAY) {
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


void register_plugin_syslogd() {
  INFO("Syslog", "Sending buffered data to syslog plugin");

  /*
    @todo
    Get dmesg (kernel logs) and send to syslog
    INFO needs to be rewritten to use kprint and kprint needs to be rewritten to buffer the data
  */

}


__attribute__((constructor))
void register_syslogd(){
  OS::register_plugin(register_plugin_syslogd, "Syslog over UDP");
}
