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

// Syslog_facility weak default printf

#pragma once
#ifndef UTIL_SYSLOG_FACILITY_HPP
#define UTIL_SYSLOG_FACILITY_HPP

#define LOG_INTERNAL  5   /* Messages generated internally by syslogd */

#include <cstdio>
#include <iostream>
#include <string>
#include <map>

#include <syslog.h>		// POSIX symbolic constants
#include <net/inet4>	// For private attribute UDPSocket*

const int MUL_VAL = 8;
const std::map<int, std::string> pri_colors = {
  { LOG_EMERG,   "\033[38;5;1m" },    // RED
  { LOG_ALERT,   "\033[38;5;160m" },  // RED (lighter)
  { LOG_CRIT,    "\033[38;5;196m" },  // RED (even lighter)
  { LOG_ERR,     "\033[38;5;208m" },  // DARK YELLOW
  { LOG_WARNING, "\033[93m" },        // YELLOW
  { LOG_NOTICE,  "\033[92m" },        // GREEN
  { LOG_INFO,    "\033[96m" },        // TURQUOISE
  { LOG_DEBUG,   "\033[94m" }         // BLUE
};
const std::string COLOR_END = "\033[0m";

class Syslog_facility {

public:

  void settings(const net::UDP::addr_t dest_ip, const net::UDP::port_t dest_port) {
    ip_ = dest_ip;
    port_ = dest_port;
  }

  net::UDP::addr_t ip() {
    return ip_;
  }

  net::UDP::port_t port() {
    return port_;
  }

	//__attribute__((weak))
  void syslog(const std::string& log_message);

  std::string facility_name() {
  	switch (facility_) {
			case LOG_KERN:
				return "KERN";
			case LOG_USER:
				return "USER";
			case LOG_MAIL:
				return "MAIL";
			case LOG_DAEMON:
				return "DAEMON";
			case LOG_AUTH:
				return "AUTH";
			case LOG_INTERNAL:
				return "INTERNAL";
			case LOG_LPR:
				return "LPR";
			case LOG_NEWS:
				return "NEWS";
			case LOG_UUCP:
				return "UUCP";
			case LOG_CRON:
				return "CRON";
			case LOG_LOCAL0:
				return "LOCAL0";
			case LOG_LOCAL1:
				return "LOCAL1";
			case LOG_LOCAL2:
				return "LOCAL2";
			case LOG_LOCAL3:
				return "LOCAL3";
			case LOG_LOCAL4:
				return "LOCAL4";
			case LOG_LOCAL5:
				return "LOCAL5";
			case LOG_LOCAL6:
				return "LOCAL6";
			case LOG_LOCAL7:
				return "LOCAL7";
			default:
				return "NONE";
		}
  }

  int calculate_pri() { return (facility_ * MUL_VAL) + priority_; }

  Syslog_facility() : facility_{LOG_USER} {}

  Syslog_facility(const char* ident, int facility) : ident_{ident}, facility_{facility} {}

  __attribute__((weak))
  ~Syslog_facility();

  bool ident_is_set() { return (ident_ not_eq nullptr); }

  void set_ident(const char* ident) { ident_ = ident; }

  const char* ident() { return ident_; }

  void set_priority(int priority) { priority_ = priority; }

  int priority() { return priority_; }

	std::string priority_name() {
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

  void set_logopt(int logopt) { logopt_ = logopt; }

  int logopt() { return logopt_; }

  void set_facility(int facility) { facility_ = facility; }

  int facility() { return facility_; }

  __attribute__((weak))
  void open_socket();

  __attribute__((weak))
  void close_socket();

  __attribute__((weak))
  void send_udp_data(const std::string&);

private:
  const char* ident_ = nullptr;
  int facility_;
  int priority_;
  int logopt_;

  net::UDP::addr_t ip_{0};
  net::UDP::port_t port_{0};
  net::UDPSocket* sock_ = nullptr;
};

#endif
