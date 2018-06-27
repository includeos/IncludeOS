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
#ifndef UTIL_SYSLOGD_HPP
#define UTIL_SYSLOGD_HPP

#include <cstdio>
#include <string>
#include <regex>
#include <stdarg.h>

#include <syslog.h> // POSIX symbolic constants
#include "syslog_facility.hpp"

class Syslog {
public:
  static void set_facility(std::unique_ptr<Syslog_facility> facility) {
    fac_ = std::move(facility);
  }

  static void settings(const net::Addr& dest_ip, const uint16_t dest_port) {
    fac_->settings(dest_ip, dest_port);
  }

  static const net::Addr& ip() {
    return fac_->ip();
  }

  static uint16_t port() {
    return fac_->port();
  }

  // va_list arguments (POSIX)
  static void syslog(int priority, const char* message, va_list args);

  __attribute__ ((format (printf, 2, 3)))
  static void syslog(int priority, const char* buf, ...);

  static void openlog(const char* ident, int logopt, int facility);

  static void closelog();

  static bool valid_priority(int priority) noexcept {
    return not ((priority < LOG_EMERG) or (priority > LOG_DEBUG));
  }

  static bool valid_logopt(int logopt) noexcept {
    return (logopt & LOG_PID or logopt & LOG_CONS or logopt & LOG_NDELAY or
      logopt & LOG_ODELAY or logopt & LOG_NOWAIT or logopt & LOG_PERROR);
  }

  static bool valid_facility(int facility) noexcept {
    return ( (facility >= LOG_KERN and facility <= LOG_CRON ) or (facility >= LOG_LOCAL0 and facility <= LOG_LOCAL7) );
  }

private:
  static std::unique_ptr<Syslog_facility> fac_;

}; // < Syslog

#endif
