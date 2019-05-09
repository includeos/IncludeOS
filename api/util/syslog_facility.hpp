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

#pragma once
#ifndef UTIL_SYSLOG_FACILITY_HPP
#define UTIL_SYSLOG_FACILITY_HPP

#define LOG_INTERNAL  5   /* Messages generated internally by syslogd */

#include <cstdio>
#include <string>
#include <map>

#include <syslog.h>         // POSIX symbolic constants
#include <net/udp/socket.hpp>  // For private attribute UDPSocket* in Syslog_udp

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

// Syslog_facility

class Syslog_facility {
public:
  virtual void syslog(const std::string&) = 0;
  virtual void settings(const net::Addr&, const uint16_t) = 0;
  virtual const net::Addr& ip() const noexcept = 0;
  virtual uint16_t port() const noexcept = 0;
  virtual void open_socket() = 0;
  virtual void close_socket() = 0;
  virtual std::string build_message_prefix(const std::string&) = 0;
  virtual ~Syslog_facility() {}

  Syslog_facility() {}
  Syslog_facility(const char* ident, int facility) : ident_{ident}, facility_{facility} {}

  std::string facility_name() const noexcept {
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

  inline int calculate_pri() { return (facility_ * MUL_VAL) + priority_; }
  inline bool ident_is_set() const noexcept { return (ident_ not_eq nullptr); }
  inline void set_ident(const char* ident) { ident_ = ident; }
  inline const char* ident() const noexcept { return ident_; }
  inline void set_priority(int priority) noexcept { priority_ = priority; }
  inline int priority() const noexcept { return priority_; }

  std::string priority_name() const noexcept {
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

  inline void set_logopt(int logopt) noexcept { logopt_ = logopt; }
  inline int logopt() const noexcept { return logopt_; }
  inline void set_facility(int facility) noexcept { facility_ = facility; }
  inline int facility() const noexcept { return facility_; }

private:
  const char* ident_ = nullptr;
  int facility_{LOG_USER};
  int priority_{0};
  int logopt_{0};

}; // < Syslog_facility

// Syslog_udp

class Syslog_udp : public Syslog_facility {
public:
  inline void settings(const net::Addr& dest_ip, const uint16_t dest_port) {
    ip_ = dest_ip;
    port_ = dest_port;
  }

  inline const net::Addr& ip() const noexcept {
    return ip_;
  }

  inline uint16_t port() const noexcept {
    return port_;
  }

  void syslog(const std::string& log_message);

  void open_socket();

  void close_socket();

  void send_udp_data(const std::string& data);

  std::string build_message_prefix(const std::string& binary_name);

  Syslog_udp() : Syslog_facility() {}
  Syslog_udp(const char* ident, int facility) : Syslog_facility(ident, facility) {}

  ~Syslog_udp();

private:
  net::Addr ip_{};
  uint16_t port_{0};
  net::udp::Socket* sock_ = nullptr;

}; // < Syslog_udp

// Syslog_print

class Syslog_print : public Syslog_facility {
public:
  void syslog(const std::string& log_message) override;
  void settings(const net::Addr&, const uint16_t) override {}
  const net::Addr& ip() const noexcept override { return net::Addr::addr_any; }
  uint16_t port() const noexcept override { return 0; }
  void open_socket() override {}
  void close_socket() override {}

  std::string build_message_prefix(const std::string& binary_name) override;

  Syslog_print() : Syslog_facility() {}
  Syslog_print(const char* ident, int facility) : Syslog_facility(ident, facility) {}
  ~Syslog_print() {}

}; // < Syslog_print

#endif
