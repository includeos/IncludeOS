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

#include <util/syslog_facility.hpp>
#include <net/interfaces>
#include <unistd.h> // getpid
#include <ctime>

const int TIMELEN = 32;

// Syslog_udp (plugin)

void Syslog_udp::syslog(const std::string& log_message) {
  //if (logopt() & LOG_CONS /*and priority() == LOG_ERR*/)
  //  fprintf(stdout, "%s\n", log_message.c_str());

  if (logopt() & LOG_PERROR)
    fprintf(stderr, "%s\n", log_message.c_str());

  send_udp_data(log_message);
}

void Syslog_udp::open_socket() {
  if (sock_ == nullptr)
    sock_ = &net::Interfaces::get(0).udp().bind();
}

void Syslog_udp::close_socket() {
  if (sock_) {
    sock_->close();
    sock_ = nullptr;
  }
}

void Syslog_udp::send_udp_data(const std::string& data) {
  open_socket();
  sock_->sendto( ip_, port_, data.c_str(), data.size() );
}

std::string Syslog_udp::build_message_prefix(const std::string& binary_name) {
  /* Building the log message based on RFC5424 */

  /* Header: PRI VERSION SP TIMESTAMP SP HOSTNAME SP APP-NAME SP PROCID SP MSGID */

  // First: Priority- and facility-value (PRIVAL) and Syslog-version
  std::string message = "<" + std::to_string(calculate_pri()) + ">1 ";

  // Second: Timestamp
  char timebuf[TIMELEN];
  time_t now;
  time(&now);
  strftime(timebuf, TIMELEN, "%FT%T.000Z", localtime(&now));

  // Third: Hostname ( Preferably: 1. FQDN (RFC1034) 2. Static IP address 3. Hostname 4. Dynamic IP address 5. NILVALUE (-) )
  message += std::string{timebuf} + " " + net::Interfaces::get(0).ip_addr().str() + " ";

  // Fourth: App-name, PROCID (LOG_PID) and MSGID
  message += std::string(binary_name) + " " + std::to_string(getpid()) + " UDPOUT ";

  /* Structured data: SD-element (SD-ID PARAM-NAME=PARAM-VALUE) */
  message += "- ";  // NILVALUE

  /* Message */

  // Add ident before message (buf) if is set (through openlog)
  if (ident_is_set())
    message += std::string{ident()} + " ";

  return message;
}

Syslog_udp::~Syslog_udp() {
  if (sock_)
    sock_->close();
}

// < Syslog_udp (plugin)

// Syslog_print (printf)

void Syslog_print::syslog(const std::string& log_message) {
  //if (logopt() & LOG_CONS /*and priority() == LOG_ERR*/)

  if (logopt() & LOG_PERROR)
    fprintf(stderr, "%s\n", log_message.c_str());

  printf("%s\n", log_message.c_str());
}

std::string Syslog_print::build_message_prefix(const std::string& binary_name) {
  /* PRI FAC_NAME PRI_NAME TIMESTAMP APP-NAME IDENT PROCID */

  // First: Priority- and facility-value (PRIVAL)
  std::string message = "<" + std::to_string(calculate_pri()) + "> ";

  // Second : Facility and priority/severity in plain text with colors
  message += pri_colors.at(priority()) + "<" + facility_name() + "." +
    priority_name() + "> " + COLOR_END;

  // Third: Timestamp
  char timebuf[TIMELEN];
  time_t now;
  time(&now);
  strftime(timebuf, TIMELEN, "%FT%T.000Z", localtime(&now));
  message += std::string{timebuf} + " ";

  // Fourth: App-name
  message += std::string(binary_name);

  // Fifth: Add ident if is set (through openlog)
  if (ident_is_set())
    message += " " + std::string{ident()};

  // Sixth: Add PID (PROCID) if LOG_PID is specified (through openlog)
  if (logopt() & LOG_PID)
    message += "[" + std::to_string(getpid()) + "]";

  message += ": ";

  // NEW: Was only in plugin
  // Add ident before message (buf) if is set (through openlog)
  // if (ident_is_set())
  //    message += std::string{ident()} + " ";

  return message;
}

// < Syslog_print (printf)
