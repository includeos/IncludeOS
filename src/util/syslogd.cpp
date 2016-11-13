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

Syslog_facility::~Syslog_facility() {
  
  printf("Destructor Syslog_facility\n");

  if (sock_) {
    sock_->udp().close(sock_->local_port());
    
    printf("Destructor socket\n");
  }
}

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

void Syslog_facility::open_socket() {
  if (sock_ == nullptr) {
    sock_ = &Inet4::stack<>().udp().bind();
    printf("Open socket: BINDING\n");
  }
  else {
    printf("Open socket: NOT BINDING - sock_ is not nullptr\n");
  }
}

void Syslog_facility::close_socket() {
	if (sock_) {
    sock_->udp().close(sock_->local_port());

    printf("Close socket: sock was not nullptr and is now closed\n");

	} else {

		printf("Close socket: sock is already nullptr\n");

	}
}

void Syslog_facility::send_udp_data(const std::string& data) {
  if (logopt_ & LOG_CONS /*and priority_ == LOG_ERR*/) {

		printf("LOG_CONS\n");

    std::cout << data.c_str() << '\n';
  }

  if (logopt_ & LOG_PERROR) {

  	printf("LOG_PERROR\n");

    std::cerr << data.c_str() << '\n';
  }

  open_socket();
  sock_->sendto( Inet4::stack().gateway(), UDP_PORT, data.c_str(), data.size() );

  /*auto& sock = Inet4::stack<>().udp().bind();
  sock.sendto( Inet4::stack().gateway(), UDP_PORT, data.c_str(), data.size() );*/
  // sock.sendto( {46, 31, 185, 167}, UDP_PORT, data.c_str(), data.size() );
}

// --------------------------- Syslog_kern ------------------------------

void Syslog_kern::syslog(const std::string& log_message) {

	// Just for testing:
	// printf("%s\n", log_message.c_str());

	// Send message over UDP
	send_udp_data(log_message);
}

std::string Syslog_kern::name() { return "KERN"; }

int Syslog_kern::calculate_pri() { return (LOG_KERN * MUL_VAL) + priority(); }

// --------------------------- Syslog_user ------------------------------

void Syslog_user::syslog(const std::string& log_message) {

	// printf("%s\n", log_message.c_str());

	// Send message over UDP
	send_udp_data(log_message);
}

std::string Syslog_user::name() { return "USER"; }

int Syslog_user::calculate_pri() { return (LOG_USER * MUL_VAL) + priority(); }

// --------------------------- Syslog_mail ------------------------------

void Syslog_mail::syslog(const std::string& log_message) {

	// Just for testing:
	// printf("%s\n", log_message.c_str());

	// Send message over UDP
	send_udp_data(log_message);
}

std::string Syslog_mail::name() { return "MAIL"; }

int Syslog_mail::calculate_pri() { return (LOG_MAIL * MUL_VAL) + priority(); }

// ----------------------------- Syslog ---------------------------------

std::unique_ptr<Syslog_facility> Syslog::last_open = std::make_unique<Syslog_user>();

//UDPSocket* Syslog::sock_ = nullptr;

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
  	All syslog-calls comes through here in the end, so
  	here we want to format the log-message with header and body
  */

	// % is kept if calling this with %m and no arguments

	/* If the priority is not valid, call syslog again with LOG_ERR as priority
		 and an unknown priority-message before the
		 Could also document by setting logopt to LOG_PID | LOG_CONS | LOG_PERROR, but
		 then only for this specific message */
  if (not valid_priority(priority)) {
  	syslog(LOG_ERR, "Syslog: Unknown priority %d. Message: %s", priority, buf);
    return;
  }

 	last_open->set_priority(priority);

 	/* Building the log message based on RFC5424 */

 	/* Header: PRI VERSION SP TIMESTAMP SP HOSTNAME SP APP-NAME SP PROCID SP MSGID */

 	// First: Priority- and facility-value (PRIVAL) and Syslog-version
 	std::string message = "<" + std::to_string(last_open->calculate_pri()) + ">1 ";

 	// Second: Timestamp
 	char timebuf[TIMELEN];
 	time_t now;
 	time(&now);
 	strftime(timebuf, TIMELEN, "%FT%T.000Z", localtime(&now));

 	// Third: Hostname ( Preferably: 1. FQDN (RFC1034) 2. Static IP address 3. Hostname 4. Dynamic IP address 5. NILVALUE (-) )
 	message += std::string{timebuf} + " " + Inet4::stack().ip_addr().str() + " ";

 	// Fourth: App-name, PROCID and MSGID
 	message += Service::binary_name() + " " + std::to_string(getpid()) + " UDPOUT ";

 	/* Structured data: SD-element (SD-ID PARAM-NAME=PARAM-VALUE) */
 	message += "- ";	// NILVALUE

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
  /* Handle %m (replace it with strerror(errno)) and add the message (buf) */
  std::regex m_regex{"\\%m"};
  message += std::regex_replace(buf, m_regex, strerror(errno));

 	/* Last: Send the log string */
 	last_open->syslog(message);
 	// or last_open->send_udp_data(message);
 	// or if going away from sub-facilities and only have Syslog_facility class: facility->send_udp_data(message);

  /* Building the message first implementation (with colors for facility- and severity-text) */
  /* First: Timestamp
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

  //	%m:
	//  (The message body is generated from the message (argument) and following arguments
	//  in the same manner as if these were arguments to printf(), except that the additional
	//  conversion specification %m shall be recognized;)
	//  it shall convert no arguments, shall cause the output of the error message string
	//  associated with the value of errno on entry to syslog(), and may be mixed with argument
	//  specifications of the "%n$" form. If a complete conversion specification with the m conversion
	//  specifier character is not just %m, the behavior is undefined. A trailing <newline> may be
	//  added if needed.
  
  // Fifth: Handle %m (replace it with strerror(errno)) and add the message (buf)
  std::regex m_regex{"\\%m"};
  message += std::regex_replace(buf, m_regex, strerror(errno));

  last_open->syslog(message);
  // or last_open->send_udp_data(message); if nothing is to be done in the sub-facility
  */
}

void Syslog::closelog() {
  // Back to default values:
  // ident_ = nullptr; (Will then be service-name)
  // logopt = 0;
  // facility = LOG_USER;
  openlog<Syslog_user>(nullptr, 0);
  // sock_ = nullptr;
  last_open->close_socket();
}